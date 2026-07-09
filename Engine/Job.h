#pragma once
#include "Asserts.h"
#include <atomic>
#include <type_traits>

namespace rr
{
	class JobSystem;
	struct Job;
	using JobFn = void(*)(Job* self, void* data);
	using JobFnPlain = void(*)(Job* self);
	using JobFnPlainNoSelf = void(*)();

	struct Job
	{
		JobSystem* system = nullptr;
		Job* parent = nullptr;
		Job* continuation = nullptr;
		void* data = nullptr;

		JobFn execute_fn = nullptr;

		std::atomic_int32_t unfinished_jobs{ 1 };

		void Execute()
		{
			RR_ASSERT(execute_fn != nullptr);
			execute_fn(this, data);
		}
	};

	static_assert(std::is_trivially_destructible_v<Job>);
}