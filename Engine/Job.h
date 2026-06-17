#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

namespace rr
{
	class JobSystem;
	using JobID = uint16_t;
	using JobFn = void(*)(JobSystem& sys, JobID self, void* data);

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
	using TypedJobFn = void(*)(JobSystem& sys, JobID self, T& data);

	template<typename T, TypedJobFn<T> Fn>
	void JobTrampoline(JobSystem& sys, JobID self, void* data)
	{
		Fn(sys, self, *static_cast<T*>(data));
	}

	template<typename T, TypedJobFn<T> Fn, typename ...Args>
	void SetJobPayload(Job* job, Args&& ...args)
	{
		static_assert(sizeof(T) <= Job::kDataSize);
		static_assert(alignof(T) <= alignof(Job));
		job->execute_fn = &JobTrampoline<T, Fn>;
		job->destroy_fn =
			[](void* data)
			{
				std::destroy_at(static_cast<T*>(data));
			};

		::new (job->data) T(std::forward<Args>(args)...);
	}
}