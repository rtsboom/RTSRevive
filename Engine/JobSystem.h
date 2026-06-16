#pragma once
#include <atomic>
#include <cstdint>

#include "Job.h"
#include "JobPool.h"
#include "WorkStealingQueue.h"
#include <memory>
#include <utility>
#include <thread>
#include <semaphore>
#include <cassert>
namespace rr
{
	class JobSystem
	{
		struct alignas(64) Worker
		{
			WorkStealingQueue<JobID, 1024> queue;
			FrameJobCursor frame_job_cursor;
			size_t worker_id{ 0 };
			uint64_t rng_state{ 0 };
			std::thread thread;
		};
		static_assert(alignof(Worker) == 64);

		inline static thread_local Worker* tls_self_ = nullptr;

	public:
		static constexpr size_t kMaxWorkerThreads{ 15 };

		JobSystem() = default;
		~JobSystem();

		void WorkerThreadLoop(Worker* self);
		void Initialize(size_t worker_thread_count);
		void Shutdown();

		void RunJob(JobID id);
		void WaitJob(JobID id);

		template<typename T, TypedJobFn<T> Fn, typename ...Args>
		JobID CreateJobAsChild(JobID parent, Args&&... args);

		template<typename T, TypedJobFn<T> Fn, typename ...Args>
		JobID CreateJob(Args&&... args);

		void SetContinuation(JobID before, JobID after) const noexcept;

	private:
		bool AcquireJob(JobID& id);
		bool ProcessOneJob();
		void ExecuteJob(JobID id);
		void FinishJob(JobID id);

		Job* GetJobFromID(JobID id) const noexcept;
		bool IsComplete(Job* job) const noexcept;

	private:
		FrameJobPool job_pool_;
		std::unique_ptr<Worker[]> workers_{ nullptr };
		size_t worker_count_{ 0 };

		std::atomic_bool running_{ false };
		std::atomic_int32_t sleeping_workers_{ 0 };
		std::counting_semaphore<kMaxWorkerThreads> smph_{ 0 };
	};

	template<typename T, TypedJobFn<T> Fn, typename ...Args>
	inline JobID JobSystem::CreateJobAsChild(JobID parent, Args && ...args)
	{
		static_assert(sizeof(T) <= 96);
		static_assert(alignof(T) <= 64);

		JobID id = job_pool_.Allocate(tls_self_->frame_job_cursor);
		if (id == JobID_Null)
		{
			assert(false && "The job pool is exhausted.");
			return JobID_Null;
		}

		Job* job = job_pool_.GetJob(id);
		*job = {}; // reset job 

		job->parent = parent;
		job->execute_fn = &JobTrampoline<T, Fn>;
		job->destroy_fn =
			[](void* data)
			{
				std::destroy_at(static_cast<T*>(data));
			};

		::new (job->data) T(std::forward<Args>(args)...);

		job->unfinished_jobs.store(1, std::memory_order_relaxed);
		return id;
	}

	template<typename T, TypedJobFn<T> Fn, typename ...Args>
	inline JobID JobSystem::CreateJob(Args&& ...args)
	{
		return CreateJbAsChild<T, Fn>(JobID_Null, std::forward<Args>(args)...);
	}
}