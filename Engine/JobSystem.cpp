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
			sleeping_workers_.fetch_add(1, std::memory_order_relaxed);
			if (!ProcessOneJob() &&
				running_.load(std::memory_order_acquire))
				smph_.acquire();

			sleeping_workers_.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	void JobSystem::Initialize(size_t worker_thread_count)
	{
		if (kMaxWorkerThreads < worker_thread_count)
			worker_thread_count = kMaxWorkerThreads;

		worker_count_ = worker_thread_count + 1;
		workers_ = std::make_unique<Worker[]>(worker_thread_count + 1);

		tls_self_ = &workers_[worker_thread_count];
		tls_self_->worker_id = worker_thread_count;
		tls_self_->rng_state = worker_thread_count + 1;

		running_.store(true, std::memory_order_release);

		for (size_t i{}; i < worker_thread_count; ++i)
		{
			workers_[i].worker_id = i;
			workers_[i].rng_state = i + 1;
			workers_[i].thread = std::thread(&JobSystem::WorkerThreadLoop, this, &workers_[i]);
		}

	}

	void JobSystem::Shutdown()
	{
		running_.store(false, std::memory_order_release);

		int32_t const sleepers = sleeping_workers_.load(std::memory_order_relaxed);
		for (size_t i{}; i < sleepers; ++i)
			smph_.release();

		for (size_t i{}; i < worker_count_; ++i)
		{
			auto& worker_thread = workers_[i].thread;
			if (worker_thread.joinable()) // last one is main worker, blank thread
				worker_thread.join();
		}
	}

	void JobSystem::SetContinuation(JobID before, JobID after) const noexcept
	{
		Job* job = GetJobFromID(before);
		assert(job);
		assert(job->continuation == JobID_Null); // only one continuation allowed
		
		job->continuation = after;
	}

	bool JobSystem::AcquireJob(JobID& id)
	{
		if (tls_self_->queue.Pop(id))
			return true;

		uint64_t const random = XorShift64(tls_self_->rng_state);

		for (size_t i{}; i < worker_count_; ++i)
		{
			size_t const victim = (random + i) % worker_count_;
			if (victim == tls_self_->worker_id)
				continue;

			if (workers_[victim].queue.Steal(id))
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
		while (!tls_self_->queue.Push(id)) // if queue is full
		{
			ProcessOneJob();
		}

		if (sleeping_workers_.load(std::memory_order_relaxed) > 0)
			smph_.release();
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

	Job* JobSystem::GetJobFromID(JobID id) const noexcept
	{
		return job_pool_.GetJob(id);
	}

	bool JobSystem::IsComplete(Job* job) const noexcept
	{
		assert(job);
		return job->unfinished_jobs.load(std::memory_order_acquire) == 0;
	}

}