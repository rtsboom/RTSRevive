#pragma once
#include <atomic>
#include <cstdint>

#include "Job.h"
#include "WorkStealingQueue.h"
#include "StableArena.h"
#include "Asserts.h"
#include <utility>
#include <thread>
#include <semaphore>
#include <vector>
#include <tuple>
#include <memory>
#include <type_traits>
namespace rr
{
	class JobSystem
	{
		static constexpr size_t kWorkerArenaSize = 4 * 1024 * 1024; // 4MB
		struct alignas(64) Worker
		{
			StableArena arena{ kWorkerArenaSize };
			WorkStealingQueue<Job*, 1024> queue;
			size_t id{ 0 };
			uint64_t rng_state{ 0 };

			//for stats
			uint64_t created_jobs{ 0 };
			uint64_t submitted_jobs{ 0 };
			uint64_t executed_jobs{ 0 };
			uint64_t finished_jobs{ 0 };

			// background thread, not used in main worker
			std::thread thread;
			std::binary_semaphore wake_smph{ 0 };
		};
		static_assert(alignof(Worker) == 64);

		inline static thread_local Worker* tls_self_ = nullptr;

	public:
		// Uses a 64-bit sleeping worker bitmask.
		// Therefore, the worker count must not exceed 64.
		static constexpr size_t kMaxWorkerCount{ 64 };

		JobSystem() = default;
		~JobSystem();

		void Initialize(size_t worker_count);
		void Shutdown();
		void WorkerThreadLoop(Worker* self);

		void RunJob(Job* job);
		void WaitJob(Job* job);
		void Reset();

		Job* AllocateJob();
		void SetContinuation(Job* before, Job* after) const noexcept;

		//for stats
		uint64_t GetCurrentCreatedJobs() const noexcept;
		uint64_t GetCurrentSubmittedJobs() const noexcept;
		uint64_t GetCurrentExecutedJobs() const noexcept;
		uint64_t GetCurrentFinishedJobs() const noexcept;

		uint64_t GetTotalCreatedJobs() const noexcept { return total_created_jobs_; }
		uint64_t GetTotalSubmittedJobs() const noexcept { return total_submitted_jobs_; }
		uint64_t GetTotalExecutedJobs() const noexcept { return total_executed_jobs_; }
		uint64_t GetTotalFinishedJobs() const noexcept { return total_finished_jobs_; }

	private:
		bool PushJob(Job* job);
		Job* AcquireJob();
		bool ProcessOneJob();
		void ExecuteJob(Job* job);
		void FinishJob(Job* job);

		bool IsFinished(Job* job) const noexcept;

		void WakeOneWorker();
		size_t GetTotalWorkerCount() const noexcept { return workers_.size(); }
		size_t GetBackgroundWorkerCount() const noexcept { return workers_.size() - 1; }
		void CheckMainThreadOnly() const noexcept;

	private:
		template<JobFnPlain Fn>
		Job* CreatePlainJobImpl();

		template<JobFnPlainNoSelf Fn>
		Job* CreatePlainJobNoSelfImpl();

		template<auto Fn, typename... Args>
		Job* CreateJobImpl(Args&& ...args);

		template<auto Fn, typename... Args>
		Job* CreateJobNoSelfImpl(Args&& ...args);

		template<auto Fn, typename ...Args>
		Job* CreateJobRouter(Args&& ...args);

	public:
		template<auto Fn, typename ...Args>
		Job* CreateJob(Args&& ...args);

		template<auto Fn, typename ...Args>
		Job* CreateJobAsChild(Job* parent, Args&& ...args);

	private:
		std::vector<Worker> workers_;

		std::atomic_bool running_{ false };
		std::atomic_uint64_t sleeping_worker_mask_{ 0 };

		// for stats
		uint64_t total_created_jobs_{ 0 };
		uint64_t total_submitted_jobs_{ 0 };
		uint64_t total_executed_jobs_{ 0 };
		uint64_t total_finished_jobs_{ 0 };
	};

	template<JobFnPlain Fn>
	inline Job* JobSystem::CreatePlainJobImpl()
	{
		Job* const job = AllocateJob();
		job->system = this;
		job->execute_fn = [](Job* self, void*)
			{
				Fn(self);
			};

		return job;
	}

	template<JobFnPlainNoSelf Fn>
	inline Job* JobSystem::CreatePlainJobNoSelfImpl()
	{
		Job* const job = AllocateJob();
		job->system = this;
		job->execute_fn = [](Job*, void*)
			{
				Fn();
			};

		return job;
	}

	template<auto Fn, typename ...Args>
	inline Job* JobSystem::CreateJobImpl(Args&&... args)
	{
		static_assert(std::is_invocable_v<decltype(Fn), Job*, std::decay_t<Args>&...>,
			"Job function arguments do not match the function signature.");

		using Payload = std::tuple<std::decay_t<Args>...>;
		static_assert(std::is_trivially_destructible_v<Payload>,
			"Job payload must be trivially destructible. Pass owning objects by pointer.");


		Job* const job = AllocateJob();
		job->system = this;

		job->data = tls_self_->arena.New<Payload>(std::forward<Args>(args)...);
		RR_CHECK(job->data != nullptr);

		job->execute_fn = [](Job* self, void* data)
			{
				auto& payload = *static_cast<Payload*>(data);
				std::apply([&](auto&... xs) { Fn(self, xs...); }, payload);
			};

		return job;
	}

	template<auto Fn, typename ...Args>
	inline Job* JobSystem::CreateJobNoSelfImpl(Args&&... args)
	{
		static_assert(std::is_invocable_v<decltype(Fn), std::decay_t<Args>&...>,
			"Job function arguments do not match the function signature.");

		using Payload = std::tuple<std::decay_t<Args>...>;
		static_assert(std::is_trivially_destructible_v<Payload>,
			"Job payload must be trivially destructible. Pass owning objects by pointer.");

		Job* const job = AllocateJob();
		job->system = this;

		job->data = tls_self_->arena.New<Payload>(std::forward<Args>(args)...);
		RR_CHECK(job->data != nullptr);

		job->execute_fn = [](Job*, void* data)
			{
				RR_ASSERT(data != nullptr);
				auto& payload = *static_cast<Payload*>(data);
				std::apply([&](auto&... xs) { Fn(xs...); }, payload);
			};

		return job;
	}

	template<auto Fn, typename ...Args>
	inline Job* JobSystem::CreateJobRouter(Args&&... args)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			if constexpr (std::is_invocable_v<decltype(Fn), Job*>)
				return CreatePlainJobImpl<Fn>();
			else
				return CreatePlainJobNoSelfImpl<Fn>();
		}
		else
		{
			if constexpr (std::is_invocable_v<decltype(Fn), Job*, Args&...>)
				return CreateJobImpl<Fn>(std::forward<Args>(args)...);
			else
				return CreateJobNoSelfImpl<Fn>(std::forward<Args>(args)...);
		}
	}


	template<auto Fn, typename... Args>
	inline Job* JobSystem::CreateJob(Args&&... args)
	{
		CheckMainThreadOnly();
		return CreateJobRouter<Fn>(std::forward<Args>(args)...);
	}

	template<auto Fn, typename ...Args>
	inline Job* JobSystem::CreateJobAsChild(Job* parent, Args&&... args)
	{
		RR_ASSERT(parent);
		parent->unfinished_jobs.fetch_add(1, std::memory_order_relaxed);

		Job* job = CreateJobRouter<Fn>(std::forward<Args>(args)...);
		job->parent = parent;

		return job;
	}
}