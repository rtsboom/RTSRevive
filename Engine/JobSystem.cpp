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
	void JobSystem::WorkerThreadLoop(size_t index)
	{
		tls_system = this;
		tls_worker_index = index;
		uint64_t const current_worker_bit = 1ull << index;

		while (running_.load(std::memory_order_acquire))
		{
			if (ProcessOneJob())
				continue;

			// Prevent lost wakeup.
			auto const old = sleeping_worker_mask_.fetch_or(
				current_worker_bit, std::memory_order_seq_cst);

			RR_ASSERT((old & current_worker_bit) == 0);

			// double check if the job system is still running before sleeping.
			if (!running_.load(std::memory_order_acquire))
			{
				break;
			}

			// double check if a job is available before sleeping.
			if (ProcessOneJob())
			{
				auto const old = sleeping_worker_mask_.fetch_and(
					~current_worker_bit, std::memory_order_relaxed);

				// consume the semaphore token
				// when a waker already claimed the bit and released the semaphore.
				if ((old & current_worker_bit) == 0)
					GetCurrentWorker().wake_smph.acquire();
			}
			else // no job available, go to sleep
			{
				GetCurrentWorker().wake_smph.acquire();
			}
		}
	}

	void JobSystem::Initialize(size_t worker_count)
	{
		if (worker_count == 0)
			worker_count = 1;

		if (kMaxWorkerCount < worker_count)
			worker_count = kMaxWorkerCount;

		workers_ = std::vector<Worker>(worker_count);

		for (size_t i{ 0 }; i < worker_count; ++i)
		{
			workers_[i].rng_state = i + 1; // rng state cannot be 0
		}

		running_.store(true, std::memory_order_release);

		// worker index 0 is the main thread.
		for (size_t i{ 1 }; i < GetBackgroundWorkerCount(); ++i)
		{
			workers_[i].thread = std::thread(&JobSystem::WorkerThreadLoop, this, i);
		}
	}

	void JobSystem::Shutdown()
	{
		running_.store(false, std::memory_order_release);

		// Prevent lost wakeups.
		std::atomic_thread_fence(std::memory_order_seq_cst);

		// Wake all workers that are sleeping.
		auto mask = sleeping_worker_mask_.exchange(0ull, std::memory_order_relaxed);
		while (mask != 0)
		{
			size_t const index = std::countr_zero(mask);
			mask &= mask - 1; // Clear lowest set bit.

			workers_[index].wake_smph.release();
		}

		for (auto& worker : workers_)
		{
			if (worker.thread.joinable())
				worker.thread.join();
		}
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

	Job* JobSystem::AllocateJob()
	{
		constexpr size_t cache_line_size = 64;
		void* const ptr = GetCurrentWorker().arena.Allocate(sizeof(Job), cache_line_size);
		RR_CHECK(ptr != nullptr);

		Job* const job = static_cast<Job*>(ptr);
		std::construct_at(job);

		++GetCurrentWorker().created_jobs;
		return job;
	}

	Job* JobSystem::AcquireJob()
	{
		Job* out{ nullptr };
		if (GetCurrentWorker().queue.Pop(out))
			return out;

		uint64_t const random = XorShift64(GetCurrentWorker().rng_state);

		size_t const n = GetTotalWorkerCount();
		for (size_t i{}; i < n; ++i)
		{
			size_t const victim = (random + i) % n;
			if (victim == tls_worker_index)
				continue;

			if (GetWorker(victim).queue.Steal(out))
				return out;
		}

		return nullptr;
	}


	bool JobSystem::PushJob(Job* job)
	{
		return GetCurrentWorker().queue.Push(job);
	}

	bool JobSystem::ProcessOneJob()
	{
		Job* job = AcquireJob();
		if (job != nullptr)
		{
			ExecuteJob(job);
			FinishJob(job);
			return true;
		}

		return false;
	}

	void JobSystem::ExecuteJob(Job* job)
	{
		RR_ASSERT(job->system_ == this);
		job->Execute();

		++GetCurrentWorker().executed_jobs;
	}

	void JobSystem::FinishJob(Job* job)
	{
		RR_ASSERT(job != nullptr);


		int32_t const unfinished =
			job->unfinished_.fetch_sub(1, std::memory_order_acq_rel) - 1;

		RR_ASSERT(unfinished >= 0);
		if (unfinished > 0)
			return;

		Job* const parent = job->parent;
		Job* const next = job->next_;

		++GetCurrentWorker().finished_jobs;
		if (parent != nullptr)
		{
			FinishJob(job->parent);
		}
		else
		{
			finished_root_job_count_.fetch_add(1, std::memory_order_release);
		}

		if (next != nullptr)
			RunJob(job->next_);
	}


	void JobSystem::RunJob(Job* job)
	{
		RR_ASSERT(job != nullptr);
		++GetCurrentWorker().submitted_jobs;

		RR_CHECK_MSG(PushJob(job),
			"Job queue is full. Consider increasing kWorkStealingQueueSize.");


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

	void JobSystem::WaitUntilIdle()
	{
		RR_ASSERT(IsMainThread());
		while (!IsIdle())
		{
			ProcessOneJob();
		}
	}

	bool JobSystem::IsFinished(Job const* job) const noexcept
	{
		RR_ASSERT(job);
		return job->unfinished_.load(std::memory_order_acquire) == 0;
	}

	bool JobSystem::IsIdle() const noexcept
	{
		RR_ASSERT(IsMainThread());

		auto const finished = finished_root_job_count_.load(std::memory_order_acquire);
		RR_ASSERT(created_root_job_count_ >= finished);

		return created_root_job_count_ == finished;
	}

	void JobSystem::Reset()
	{
		RR_ASSERT(IsMainThread());
		RR_CHECK(IsIdle());

		auto const created_jobs = GetCurrentCreatedJobs();
		auto const submitted_jobs = GetCurrentSubmittedJobs();
		auto const executed_jobs = GetCurrentExecutedJobs();
		auto const finished_jobs = GetCurrentFinishedJobs();

		RR_CHECK(created_jobs == submitted_jobs);
		RR_CHECK(submitted_jobs == executed_jobs);
		RR_CHECK(executed_jobs == finished_jobs);

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

		created_root_job_count_ = 0;
		finished_root_job_count_.store(0, std::memory_order_relaxed);
	}

	bool JobSystem::IsMainThread() const noexcept
	{
		return tls_worker_index == 0;
	}

	JobSystem::Worker& JobSystem::GetCurrentWorker() noexcept
	{
		RR_ASSERT(IsMainThread() || tls_system == this);

		return workers_[tls_worker_index];
	}

	void JobSystem::WakeOneWorker()
	{
		std::atomic_thread_fence(std::memory_order_seq_cst);
		uint64_t mask = sleeping_worker_mask_.load(std::memory_order_relaxed);

		while (mask != 0)
		{
			int const i = std::countr_zero(mask);
			uint64_t const target_bit = 1ull << i;

			if (sleeping_worker_mask_.compare_exchange_weak(
				mask,
				mask & ~target_bit,
				std::memory_order_relaxed,
				std::memory_order_relaxed))
			{
				workers_[i].wake_smph.release();
				return;
			}
		}
	}

}