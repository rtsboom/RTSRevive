#include "EnginePch.h"
#include "JobSystem.h"
#include "Job.h"
#include <thread>
#include <cstdint>
#include <mutex>
#include <cstdlib>
#include <cassert>
#include <memory>

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
		tls_current_worker_ = worker;
		while (running_.load(std::memory_order_acquire))
		{
			JobID job_id = AcquireJob();
			if (job_id != JobID_Null)
			{
				ExecuteJob(job_id);
				continue;
			}

			std::unique_lock lock(sleep_mutex_);
			sleep_cv_.wait(lock, [&]
				{
					return !running_.load(std::memory_order_relaxed);
				});


		}
	}

	void JobSystem::Initialize(size_t worker_thread_count)
	{
		worker_count_ = worker_thread_count + 1;
		workers_ = std::make_unique<Worker[]>(worker_thread_count + 1);

		tls_current_worker_ = &workers_[worker_thread_count];
		tls_current_worker_->worker_id = worker_thread_count;
		tls_current_worker_->rng_state = worker_thread_count + 1;

		running_.store(true, std::memory_order_release);

		worker_threads_.reserve(worker_thread_count);
		for (size_t i{}; i < worker_thread_count; ++i)
		{
			workers_[i].worker_id = i;
			workers_[i].rng_state = i + 1;
			worker_threads_.emplace_back(&JobSystem::WorkerThreadLoop, this, &workers_[i]);
		}

	}

	void JobSystem::Shutdown()
	{
		running_.store(false, std::memory_order_release);
		sleep_cv_.notify_all();

		for (auto& thread : worker_threads_)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

	JobID JobSystem::AcquireJob()
	{
		JobID job_id;
		if (tls_current_worker_->queue.Pop(job_id))
		{
			return job_id;
		}

		if (worker_count_ <= 1) // main thread only
		{
			return JobID_Null;
		}


		uint64_t const random = XorShift64(tls_current_worker_->rng_state);
		
		for (size_t i{}; i < worker_count_; ++i)
		{
			size_t const victim = (random + i) % worker_count_;
			if (victim == tls_current_worker_->worker_id)
				continue;

			if (workers_[victim].queue.Steal(job_id))
			{
				return job_id;
			}
		}

		return JobID_Null;
	}

	void JobSystem::ExecuteJob(JobID id)
	{
		Job* job = GetJobFromID(id);
		assert(job);
		(job->execute_fn)(id, job->data);
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
			{
				FinishJob(job->parent);
			}

			if (job->continuation != JobID_Null)
			{
				RunJob(job->continuation);
			}
			job->destroy_fn(job->data);
		}
	}

	void JobSystem::RunJob(JobID id)
	{
		assert(id != JobID_Null);
		tls_current_worker_->queue.Push(id);
	}

	void JobSystem::WaitJob(JobID id)
	{
		assert(id != JobID_Null);
		Job* job = GetJobFromID(id);
		while (job->unfinished_jobs.load(std::memory_order_acquire) > 0)
		{
			JobID const next_job_id = AcquireJob();
			if (next_job_id != JobID_Null)
			{
				ExecuteJob(next_job_id);
			}
		}
	}

}