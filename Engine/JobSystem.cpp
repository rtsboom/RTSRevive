#include "pch.h"
#include "JobSystem.h"
#include "Job.h"
#include <thread>
#include <cstdint>
#include <cassert>
#include <memory>
#include <Windows.h>

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

		constexpr uint32_t spin_threshold{ 64 };
		uint32_t spin{ 0 };
		while (running_.load(std::memory_order_acquire))
		{
			if (ProcessOneJob())
			{
				spin = 0;
				continue;
			}

			++spin;
			if (spin < spin_threshold)
			{
				_mm_pause();
				continue;
			}

			spin = 0;

			// pair with WakeOneWorker()
			sleeping_workers_.fetch_add(1, std::memory_order_seq_cst);

			if (ProcessOneJob() || !running_.load(std::memory_order_acquire))
			{
				// cancel sleep, we have work to do or shutting down
				auto test_old = sleeping_workers_.fetch_sub(1, std::memory_order_relaxed);
				assert(test_old >= 0);
				continue;
			}

			// sleep
			smph_.acquire();
		}
	}

	void JobSystem::Initialize(size_t worker_count)
	{
		if (worker_count == 0)
			worker_count = 1;

		if (kMaxWorkerCount < worker_count)
			worker_count = kMaxWorkerCount;

		workers_ = std::vector<Worker>(worker_count);
		tls_self_ = &workers_.front();


		for (size_t i{}; i < worker_count; ++i)
		{
			workers_[i].id = i;
			workers_[i].rng_state = i + 1; // rng state cannot be 0
		}

		running_.store(true, std::memory_order_release);
		for (size_t i{ 1 }; i < worker_count; ++i)
		{
			workers_[i].thread = std::thread(&JobSystem::WorkerThreadLoop, this, &workers_[i]);
		}
	}

	void JobSystem::Shutdown()
	{
		running_.store(false, std::memory_order_release);
		smph_.release(GetBackgroundWorkerCount());

		for (auto& worker : workers_)
		{
			if (worker.thread.joinable())
				worker.thread.join();
		}
	}

	void JobSystem::SetContinuation(JobID before, JobID after) const noexcept
	{
		Job* job = GetJobFromID(before);
		assert(job);
		assert(job->continuation == JobID_Null); // only one continuation allowed

		job->continuation = after;
	}

	uint64_t JobSystem::GetTotalExecutedJobs() const noexcept
	{
		uint64_t total = 0;
		for (auto const& worker : workers_)
		{
			total += worker.executed_jobs;
		}
		return total;
	}

	uint64_t JobSystem::GetTotalPushedJobs() const noexcept
	{
		uint64_t total = 0;
		for (auto const& worker : workers_)
		{
			total += worker.pushed_jobs;
		}
		return total;
	}

	uint64_t JobSystem::GetCurrentExecutedJobs() const noexcept
	{
		uint64_t total = GetTotalExecutedJobs();
		uint64_t const current_total = total - prev_total_executed_jobs_;
		return current_total;
	}

	uint64_t JobSystem::GetCurrentPushedJobs() const noexcept
	{
		uint64_t total = GetTotalPushedJobs();
		uint64_t const current_total = total - prev_total_pushed_jobs_;
		return current_total;
	}

	bool JobSystem::PushJob(JobID id)
	{
		if (tls_self_->queue.Push(id))
		{
			++tls_self_->pushed_jobs;
			return true;
		}

		return false;
	}

	bool JobSystem::AcquireJob(JobID& out)
	{
		if (tls_self_->queue.Pop(out))
			return true;

		uint64_t const random = XorShift64(tls_self_->rng_state);

		size_t const n = GetTotalWorkerCount();
		for (size_t i{}; i < n; ++i)
		{
			size_t const victim = (random + i) % n;
			if (victim == tls_self_->id)
				continue;

			if (workers_[victim].queue.Steal(out))
				return true;
		}

		return false;
	}

	bool JobSystem::ProcessOneJob()
	{
		JobID id;
		if (AcquireJob(id))
		{
			ExecuteJob(id);
			return true;
		}

		return false;
	}

	void JobSystem::ExecuteJob(JobID id)
	{
		Job* job = GetJobFromID(id);
		assert(job);
		(job->execute_fn)(*this, id, job->data);
		FinishJob(id);

		// for stats
		++tls_self_->executed_jobs;
	}

	void JobSystem::FinishJob(JobID id)
	{
		Job* job = GetJobFromID(id);
		assert(job);

		uint32_t const unfinished = job->unfinished_jobs.fetch_sub(1, std::memory_order_acq_rel) - 1;
		if (unfinished == 0)
		{
			if (job->parent != JobID_Null)
				FinishJob(job->parent);

			if (job->continuation != JobID_Null)
				RunJob(job->continuation);

			job->destroy_fn(job->data);
		}
	}


	void JobSystem::RunJob(JobID id)
	{
		assert(id != JobID_Null);
		while (!PushJob(id)) // if queue is full
		{
			ProcessOneJob();
		}

		WakeOneWorker();
	}

	void JobSystem::WaitJob(JobID id)
	{
		assert(id != JobID_Null);
		Job* job = GetJobFromID(id);

		while (!IsComplete(job))
		{
			ProcessOneJob();
		}
	}

	void JobSystem::FrameReset()
	{
		job_pool_.Reset();
		prev_total_executed_jobs_ = GetTotalExecutedJobs();
		prev_total_pushed_jobs_ = GetTotalPushedJobs();
	}

	JobID JobSystem::AllocateJob()
	{
		JobID id = job_pool_.Allocate(tls_self_->frame_job_cursor);
		assert(id != JobID_Null && "The job pool is exhausted.");

		return id;
	}

	Job* JobSystem::GetJobFromID(JobID id) const noexcept
	{
		assert(id != JobID_Null);
		return job_pool_.GetJob(id);
	}

	bool JobSystem::IsComplete(Job* job) const noexcept
	{
		assert(job);
		return job->unfinished_jobs.load(std::memory_order_acquire) == 0;
	}

	void JobSystem::WakeOneWorker()
	{
		// Prevent Store-Load reordering (PushJob -> Load) 
		std::atomic_thread_fence(std::memory_order_seq_cst);
		int32_t n = sleeping_workers_.load(std::memory_order_relaxed);

		while (n > 0)
		{
			if (sleeping_workers_.compare_exchange_weak(
				n,
				n - 1,
				std::memory_order_relaxed,
				std::memory_order_relaxed))
			{
				smph_.release();
				return;
			}
		}
	}

}