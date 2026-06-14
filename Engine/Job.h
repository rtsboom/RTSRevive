#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <limits>

namespace rr
{
	using JobID = uint16_t;
	using JobFn = void(*)(JobID self, void* data);

	constexpr JobID JobID_Null = std::numeric_limits<JobID>::max();
	struct alignas(64) Job
	{
		static constexpr size_t kDataSize = 96;
		std::byte data[kDataSize] = {};

		std::atomic_int32_t unfinished_jobs{ 0 };
		JobID parent = JobID_Null;
		JobID continuation = JobID_Null;

		JobFn execute_fn = nullptr;
		void (*destroy_fn)(void*) = nullptr;
	};
	static_assert(alignof(Job) == 64);
	static_assert(sizeof(Job) == 128);


	template<typename T>
	using TypedJobFn = void(*)(JobID self, T& data);

	template<typename T, TypedJobFn<T> Fn>
	void JobTrampoline(JobID self, void* data)
	{
		Fn(self, *static_cast<T*>(data));
	}
}