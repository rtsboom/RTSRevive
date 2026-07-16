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
#include <type_traits>
#include <functional>
namespace rr
{
	class JobSystem
	{
		static constexpr size_t kWorkerArenaSize = 4 * 1024 * 1024; // 4MB
		struct alignas(64) Worker
		{
			StableArena arena{ kWorkerArenaSize };
			WorkStealingQueue<Job*, 1024> queue;
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

		inline static thread_local size_t tls_worker_index{ 0 };
		inline static thread_local JobSystem* tls_system{ nullptr };

	public:
		// Uses a 64-bit sleeping worker bitmask.
		// Therefore, the worker count must not exceed 64.
		static constexpr size_t kMaxWorkerCount{ 64 };

		JobSystem() = default;
		~JobSystem();

		void Initialize(size_t worker_count);
		void Shutdown();
		void WorkerThreadLoop(size_t index);

		void RunJob(Job* job);
		void WaitJob(Job* job);
		bool IsFinished(Job* job) const noexcept;
		void Reset();


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
		Job* AllocateJob();
		Job* AcquireJob();
		bool PushJob(Job* job);
		bool ProcessOneJob();
		void ExecuteJob(Job* job);
		void FinishJob(Job* job);

	private:
		bool IsMainThread() const noexcept;
		Worker& GetCurrentWorker() noexcept;
		Worker& GetWorker(size_t index) noexcept { return workers_[index]; }
		void WakeOneWorker();
		size_t GetTotalWorkerCount() const noexcept { return workers_.size(); }
		size_t GetBackgroundWorkerCount() const noexcept { return workers_.size() - 1; }

	private:
		template<JobFnNoPayload Fn>
		Job* CreateJobNoPayloadImpl();

		template<auto Fn, typename... Args>
		Job* CreateJobImpl(Args&& ...args);

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

	template<JobFnNoPayload Fn>
	inline Job* JobSystem::CreateJobNoPayloadImpl()
	{
		Job* const job = AllocateJob();
		job->system_ = this;
		job->execute_ = [](Job* ctx, void*)
			{
				std::invoke(Fn, ctx);
			};

		return job;
	}

	template<auto Fn, typename ...Args>
	inline Job* JobSystem::CreateJobImpl(Args&&... args)
	{
		using Payload = std::tuple<std::decay_t<Args>...>;
		static_assert(
			std::is_trivially_destructible_v<Payload>,
			"Job payload must be trivially destructible.");

		static_assert(
			std::is_invocable_v<
			decltype(Fn),
			Job*,
			std::decay_t<Args>&...>,
			"Job function arguments do not match the function signature.");


		Job* const job = AllocateJob();
		job->system_ = this;

		job->payload_ = GetCurrentWorker().arena.New<Payload>(std::forward<Args>(args)...);
		RR_ASSERT(job->payload_ != nullptr);

		job->execute_ = [](Job* ctx, void* data)
			{
				auto& payload = *static_cast<Payload*>(data);
				std::apply([&](auto&... xs) { std::invoke(Fn, ctx, xs...); }, payload);
			};

		return job;
	}


	template<auto Fn, typename... Args>
	inline Job* JobSystem::CreateJob(Args&&... args)
	{
		RR_ASSERT(IsMainThread());
		if constexpr (sizeof...(Args) == 0)
		{
			return CreateJobNoPayloadImpl<Fn>();
		}
		else
		{
			return CreateJobImpl<Fn>(std::forward<Args>(args)...);
		}
	}

	template<auto Fn, typename ...Args>
	inline Job* JobSystem::CreateJobAsChild(Job* parent, Args&&... args)
	{
		RR_ASSERT(parent);
		parent->unfinished_.fetch_add(1, std::memory_order_relaxed);

		Job* job;
		if constexpr (sizeof...(Args) == 0)
		{
			job = CreateJobNoPayloadImpl<Fn>();
		}
		else
		{
			job = CreateJobImpl<Fn>(std::forward<Args>(args)...);
		}

		job->parent = parent;
		return job;
	}

	// from Job class
	template<auto Fn, typename ...Args>
	inline Job* rr::Job::CreateChild(Args ...args)
	{
		return system_->CreateJobAsChild<Fn>(this, std::forward<Args>(args)...);
	}

}