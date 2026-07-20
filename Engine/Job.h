#pragma once
#include "Asserts.h"
#include "StableArena.h"
#include <atomic>
#include <type_traits>

namespace rr
{
	class JobSystem;
	class Job;
	using JobFn = void(*)(Job* ctx, void* payload);
	using JobFnNoPayload = void(*)(Job* ctx);

	class Job
	{
		friend JobSystem;
	private:
		JobSystem* system_ = nullptr;
		Job* parent = nullptr;
		Job* next_ = nullptr;

		JobFn execute_ = nullptr;
		void* payload_ = nullptr;

		std::atomic_int32_t unfinished_{ 1 };

	private:
		void Execute()
		{
			RR_ASSERT(execute_ != nullptr);
			execute_(this, payload_);
		}

	public:
		template <auto Fn, typename... Args>
		Job* CreateChild(Args... args);

		void Run(Job* job);
		void SetNext(Job* job);
		bool IsFinished() const noexcept;
	};

	static_assert(std::is_trivially_destructible_v<Job>);
}