#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace rr
{
	using JobID = uint16_t;

	struct alignas(64) Job
	{
		static constexpr size_t kDataSize = 128 - 32;
		std::byte data[kDataSize];

		std::atomic_int32_t unfinished_jobs;
		uint32_t generation;
		JobID parent_job_idx;
		JobID continuation_job_idx;
	
		
		void (*execute_fn)(Job*);
		void (*destroy_fn)(void*);

		// TODO: Move to CreateJob or free function 
		template <typename T>
		void SetDestroyFunction()
		{
			static_assert(alignof(T) <= 64);
			static_assert(sizeof(T) <= kDataSize);
			destroy_fn = [](void* ptr)
				{
					static_cast<T*>(ptr)->~T();
				};
		}

		// TODO: Move to CreateJob
		template <typename T, typename... Args>
		T* EmplaceParams(Args&&... args)
		{
			static_assert(alignof(T) <= 64);
			static_assert(sizeof(T) <= kDataSize);
			return ::new (static_cast<void*>(data)) T(std::forward<Args>(args)...);
		}
	};
	static_assert(alignof(Job) == 64);
	static_assert(sizeof(Job) == 128);
}