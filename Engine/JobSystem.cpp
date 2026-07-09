#include "pch.h"
#include "JobSystem.h"
#include "Job.h"
#include <thread>
#include <cstdint>
#include <cassert>
#include <memory>
#include <Windows.h>
#include <atomic>
#include <bit>
#include <vector>
#include "Asserts.h"

static uint64_t XorShift64(uint64_t& state)
{
	uint64_t x = state;
	assert(x != 0);

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	state = x;
	return x;
}

namespace rr
{
	JobSystem::~JobSystem()
	{
		if (running_)
		{
			Shutdown();
		}
	}
	void JobSystem::WorkerThreadLoop(Worker* worker)
	{
		tls_self_ = worker;
		uint64_t const current_worker_mask = 1ull << worker->id;

		while (running_.load(std::memory_order_acquire))
		{
			if (ProcessOneJob())
				continue;

			// pair with WakeOneWorker()
			sleeping_worker_mask_.fetch_or(current_worker_mask, std::memory_order_seq_cst);

			if (ProcessOneJob() || !running_.load(std::memory_order_acquire))
			{
				sleeping_worker_mask_.fetch_and(~current_worker_mask, std::memory_order_relaxed);
				continue;
			}

			// sleep
			worker->wake_smph.acquire();
		}
	}

	void JobSystem::Initialize(size_t worker_count)
	{
		if (worker_count == 0)
			worker_count = 1;

		if (kMaxWorkerCount < worker_count)
			worker_count = kMaxWorkerCount;

		workers_ = std::vector<Worker>(worker_count);
		tls_self_ = &workers_.back();


		for (size_t i{}; i < worker_count; ++i)
		{
			workers_[i].id = i;
			workers_[i].rng_state = i + 1; // rng state cannot be 0
		}

		running_.store(true, std::memory_order_release);
		for (size_t i{}; i < GetBackgroundWorkerCount(); ++i)
		{
			workers_[i].thread = std::thread(&JobSystem::WorkerThreadLoop, this, &workers_[i]);
		}
	}

	void JobSystem::Shutdown()
	{
		running_.store(false, std::memory_order_release);

		uint64_t mask = sleeping_worker_mask_.exchange(0ull, std::memory_order_acq_rel);


		for (size_t i{}; i < GetBackgroundWorkerCount(); ++i)
		{
			workers_[i].wake_smph.release();
		}

		for (auto& worker : workers_)
		{
			if (worker.thread.joinable())
				worker.thread.join();
		}
	}

	void JobSystem::SetContinuation(Job* before, Job* after) const noexcept
	{
		// only one continuation allowed
		RR_ASSERT(before != nullptr && before->continuation == nullptr);

		before->continuation = after;
	}

	uint64_t JobSystem::GetCurrentCreatedJobs() const noexcept
	{
		uint64_t total = 0;
		for (auto const& worker : workers_)
		{
			total += worker.created_jobs;
		}
		return total;
	}

	uint64_t JobSystem::GetCurrentSubmittedJobs() const noexcept
	{
		uint64_t total = 0;
		for (auto const& worker : workers_)
		{
			total += worker.submitted_jobs;
		}
		return total;
	}

	uint64_t JobSystem::GetCurrentExecutedJobs() const noexcept
	{
		uint64_t total = 0;
		for (auto const& worker : workers_)
		{
			total += worker.executed_jobs;
		}

		return total;
	}

	uint64_t JobSystem::GetCurrentFinishedJobs() const noexcept
	{
		uint64_t total = 0;
		for (auto const& worker : workers_)
		{
			total += worker.finished_jobs;
		}
		return total;
	}


	bool JobSystem::PushJob(Job* job)
	{
		return tls_self_->queue.Push(job);
	}

	Job* JobSystem::AcquireJob()
	{
		Job* out{ nullptr };
		if (tls_self_->queue.Pop(out))
			return out;

		uint64_t const random = XorShift64(tls_self_->rng_state);

		size_t const n = GetTotalWorkerCount();
		for (size_t i{}; i < n; ++i)
		{
			size_t const victim = (random + i) % n;
			if (victim == tls_self_->id)
				continue;

			if (workers_[victim].queue.Steal(out))
				return out;
		}

		return nullptr;
	}

	bool JobSystem::ProcessOneJob()
	{
		Job* job = AcquireJob();
		if (job != nullptr)
		{
			ExecuteJob(job);
			return true;
		}

		return false;
	}

	void JobSystem::ExecuteJob(Job* job)
	{
		RR_ASSERT(job->system == this);

		++tls_self_->executed_jobs;
		job->Execute();

		// IMPORTANT:
		// FinishJob() must be the last step of job execution.
		// WaitJob() may return immediately after Finish() completes the root job, so
		// no observable side effects should occur after this call.

		FinishJob(job);
	}

	void JobSystem::FinishJob(Job* job)
	{
		RR_ASSERT(job != nullptr);


		int32_t const unfinished =
			job->unfinished_jobs.fetch_sub(1, std::memory_order_acq_rel) - 1;

		RR_ASSERT(unfinished >= 0);

		if (unfinished == 0)
		{
			Job* const continuation = job->continuation;
			Job* const parent = job->parent;

			if (continuation != nullptr)
				RunJob(job->continuation);

			if (parent != nullptr)
				FinishJob(job->parent);

			++tls_self_->finished_jobs;
		}
	}


	void JobSystem::RunJob(Job* job)
	{
		RR_ASSERT(job != nullptr);
		++tls_self_->submitted_jobs;

		while (!PushJob(job)) // if queue is full
		{
			ProcessOneJob();
		}

		WakeOneWorker();
	}

	void JobSystem::WaitJob(Job* job)
	{
		RR_CHECK(job != nullptr);

		while (!IsFinished(job))
		{
			ProcessOneJob();
		}
	}

	void JobSystem::Reset()
	{
		auto const created_jobs = GetCurrentCreatedJobs();
		auto const submitted_jobs = GetCurrentSubmittedJobs();
		auto const executed_jobs = GetCurrentExecutedJobs();
		auto const finished_jobs = GetCurrentFinishedJobs();

		RR_CHECK(created_jobs == submitted_jobs);
		RR_CHECK(submitted_jobs == executed_jobs);

		// finished_jobs is a debug-only statistic.
		// WaitJob() returns when Job::unfinished_jobs reaches zero, before finished_jobs is updated.
		// Do not use finished_jobs for Reset() consistency checks.

		for (auto& worker : workers_)
		{
			worker.arena.Rewind();
			worker.created_jobs = 0;
			worker.submitted_jobs = 0;
			worker.executed_jobs = 0;
			worker.finished_jobs = 0;
		}

		total_created_jobs_ += created_jobs;
		total_submitted_jobs_ += submitted_jobs;
		total_executed_jobs_ += executed_jobs;
		total_finished_jobs_ += finished_jobs;
	}

	Job* JobSystem::AllocateJob()
	{
		constexpr size_t cache_line_size = 64;
		void* const ptr = tls_self_->arena.Allocate(sizeof(Job), cache_line_size);
		RR_CHECK(ptr != nullptr);

		Job* const job = static_cast<Job*>(ptr);
		std::construct_at(job);

		++tls_self_->created_jobs;
		return job;
	}

	bool JobSystem::IsFinished(Job* job) const noexcept
	{
		RR_ASSERT(job);
		return job->unfinished_jobs.load(std::memory_order_acquire) == 0;
	}

	void JobSystem::WakeOneWorker()
	{
		// Prevent Store-Load reordering (PushJob -> Load) 
		std::atomic_thread_fence(std::memory_order_seq_cst);
		uint64_t mask = sleeping_worker_mask_.load(std::memory_order_relaxed);

		while (mask != 0)
		{
			int const i = std::countr_zero(mask);
			uint64_t const target_mask = 1ull << i;

			if (sleeping_worker_mask_.compare_exchange_weak(
				mask,
				mask & ~target_mask,
				std::memory_order_relaxed,
				std::memory_order_relaxed))
			{
				workers_[i].wake_smph.release();
				return;
			}
		}
	}

	void JobSystem::CheckMainThreadOnly() const noexcept
	{
		RR_ASSERT(tls_self_ == &workers_.back());
	}

}