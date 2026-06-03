#include "EnginePch.h"
#include "JobSystem.h"
#include <memory>
#include <atomic>
#include <cstdint>
#include <thread>
#include <cassert>
#include <cstdlib>
namespace rr
{
	struct alignas(64) Worker
	{
		std::thread thread;
		uint32_t worker_index;
		uint32_t job_alloc_offset;
		uint32_t job_alloc_count;
		std::atomic_uint32_t bottom;
		JobID job_queue[JobSystem::kJobsPerWorker];

		alignas(64) std::atomic_uint32_t top;
	};
	static_assert(alignof(Worker) == 64);
	static_assert(sizeof(Worker) % 64 == 0);
}

namespace rr::JobSystem
{
	std::unique_ptr<Job[]> g_job_pool;
	size_t g_job_pool_size;
	std::unique_ptr<Worker[]> g_workers;

	thread_local Worker* t_worker{};

	uint32_t g_worker_count{};
	bool g_shutdown{};

	// Worker
	void WorkerMain(Worker* worker);

	// Job allocation
	JobID AllocateJob();
	Job* GetJob(JobID id);

	// Job creation
	JobID CreateJob(JobFn fn, void* data);
	JobID CreateChildJob(JobID parent, JobFn fn, void* data);
	void SetContinuation(JobID job, JobID continuation);

	// Queue
	void Push(Worker* worker, JobID job);
	JobID Pop(Worker* worker);
	JobID Steal(Worker* victim);
	JobID AcquireJob();

	// Execution
	void Execute(JobID job);
	void Finish(JobID job);

	// Waiting
	bool IsComplete(JobID job);
	void Wait(JobID job);

	static void JobSystem::SetCurrentWorker(Worker* worker)
	{
		t_worker = worker;
	}
	static Worker* JobSystem::GetCurrentWorker()
	{
		assert(t_worker != nullptr, "Worker is null");
		return t_worker;
	}

	static bool JobSystem::ProcessOneJob()
	{
		JobID job = AcquireJob();
		if (job == JobID_Null)
			return false;

		Execute(job);
		return true;
	}

	static Worker* JobSystem::PickVictimWorker()
	{
		Worker* self = GetCurrentWorker();

		assert(g_worker_count > 1);
		uint32_t victim = rand() % (g_worker_count - 1);

		if (self->worker_index <= victim)
			++victim;

		return &g_workers[victim];
	}

	void JobSystem::WorkerMain(Worker* worker)
	{
		SetCurrentWorker(worker);

		while (!g_shutdown)
		{
			if (!ProcessOneJob())
				std::this_thread::yield();
		}
	}

	JobID JobSystem::AllocateJob()
	{
		Worker* self = GetCurrentWorker();
		uint32_t job_id = self->job_alloc_count++ & (kJobsPerWorker - 1);
		job_id += self->job_alloc_offset;

		assert(job_id < g_job_pool_size);
		assert(IsComplete(static_cast<JobID>(job_id)));

		return static_cast<JobID>(job_id);
	}

	Job* JobSystem::GetJob(JobID id)
	{
		assert(id != JobID_Null);
		assert(id < g_job_pool_size);
		return &g_job_pool[id];
	}

	JobID JobSystem::GetJobID(Job* job)
	{
		assert(job >= g_job_pool.get());
		assert(job < g_job_pool.get() + g_job_pool_size);
		return static_cast<JobID>(job - g_job_pool.get());
	}

	JobID JobSystem::AcquireJob()
	{
		Worker* self = GetCurrentWorker();

		JobID job = Pop(self);
		if (job != JobID_Null)
			return job;

		if (g_worker_count <= 1)
			return JobID_Null;

		Worker* victim = PickVictimWorker();
		return Steal(victim);
	}

	void JobSystem::Execute(JobID id)
	{
		Job* job = GetJob(id);
		(job->fn)(job);
		Finish(id);
	}

	void JobSystem::Finish(JobID id)
	{
		Job* job = GetJob(id);

		uint32_t old = job->unfinished_jobs.fetch_sub(1, std::memory_order_acq_rel);
		assert(old > 0);

		uint32_t remaining = old - 1;
		if (remaining > 0)
			return;

		if (job->parent != JobID_Null)
			Finish(job->parent);

		if (job->continuation != JobID_Null)
			RunJob(job->continuation);
	}

	bool JobSystem::IsComplete(JobID job)
	{
		assert(job != JobID_Null);
		return GetJob(job)->unfinished_jobs.load(std::memory_order_acquire) == 0;
	}

	void JobSystem::Wait(JobID job)
	{
		while (!IsComplete(job))
		{
			if (!ProcessOneJob())
				std::this_thread::yield();
		}
	}




	void JobSystem::Initialize(uint32_t worker_count)
	{
		// TODO
	}

	void JobSystem::Shutdown()
	{
		// TODO
	}

	void JobSystem::RunJob(JobID job)
	{
		assert(job != JobID_Null);
		Worker* self = GetCurrentWorker();
		Push(self, job);
	}
}
