#pragma once
#include <atomic>
#include <cstdint>

#include "Job.h"
#include "JobPool.h"
#include "WorkStealingQueue.h"
#include <utility>
#include <thread>
#include <semaphore>
#include <vector>
namespace rr
{
	class JobSystem
	{
		struct alignas(64) Worker
		{
			WorkStealingQueue<JobID, 1024> queue;
			FrameJobCursor frame_job_cursor;
			size_t id{ 0 };
			uint64_t rng_state{ 0 };

			//for stat
			uint64_t executed_jobs{ 0 };
			uint64_t pushed_jobs{ 0 };

			// background thread, not used in main worker
			std::thread thread;
		};
		static_assert(alignof(Worker) == 64);

		inline static thread_local Worker* tls_self_ = nullptr;

	public:
		static constexpr size_t kMaxWorkerCount{ 32 };

		JobSystem() = default;
		~JobSystem();

		void WorkerThreadLoop(Worker* self);
		void Initialize(size_t worker_count);
		void Shutdown();

		void RunJob(JobID id);
		void WaitJob(JobID id);
		void FrameReset();

		template<typename T, TypedJobFn<T> Fn, typename ...Args>
		JobID CreateJobAsChild(JobID parent, Args&&... args);

		template<typename T, TypedJobFn<T> Fn, typename ...Args>
		JobID CreateJob(Args&&... args);

		void SetContinuation(JobID before, JobID after) const noexcept;

		//for stats
		uint64_t GetTotalExecutedJobs() const noexcept;
		uint64_t GetTotalPushedJobs() const noexcept;
		uint64_t GetCurrentExecutedJobs() const noexcept;
		uint64_t GetCurrentPushedJobs() const noexcept;

	private:
		bool PushJob(JobID id);
		bool AcquireJob(JobID& out);
		bool ProcessOneJob();
		void ExecuteJob(JobID id);
		void FinishJob(JobID id);

		JobID AllocateJob();
		Job* GetJobFromID(JobID id) const noexcept;
		bool IsComplete(Job* job) const noexcept;

		void WakeOneWorker();
		size_t GetTotalWorkerCount() const noexcept { return workers_.size(); }
		size_t GetBackgroundWorkerCount() const noexcept { return workers_.size() - 1; }

	private:
		FrameJobPool job_pool_;
		std::vector<Worker> workers_;

		std::atomic_bool running_{ false };
		std::atomic_int32_t sleeping_workers_{ 0 };
		std::counting_semaphore<kMaxWorkerCount * 2> smph_{ 0 };

		// for stats
		uint64_t prev_total_pushed_jobs_{ 0 };
		uint64_t prev_total_executed_jobs_{ 0 };
	};



	template<typename T, TypedJobFn<T> Fn, typename ...Args>
	inline JobID JobSystem::CreateJobAsChild(JobID parent, Args&& ...args)
	{
		JobID current = AllocateJob();
		Job* current_job = GetJobFromID(current);
		Job* parent_job = GetJobFromID(parent);
		parent_job->unfinished_jobs.fetch_add(1, std::memory_order_relaxed);

		current_job->parent = parent;
		current_job->continuation = JobID_Null;
		SetJobPayload<T, Fn>(current_job, std::forward<Args>(args)...);

		current_job->unfinished_jobs.store(1, std::memory_order_relaxed);

		return current;
	}

	template<typename T, TypedJobFn<T> Fn, typename ...Args>
	inline JobID JobSystem::CreateJob(Args&& ...args)
	{
		JobID current = AllocateJob();
		Job* current_job = GetJobFromID(current);

		current_job->parent = JobID_Null;
		current_job->continuation = JobID_Null;
		SetJobPayload<T, Fn>(current_job, std::forward<Args>(args)...);

		current_job->unfinished_jobs.store(1, std::memory_order_relaxed);

		return current;
	}
}