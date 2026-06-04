#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <utility>

namespace rr
{
	using JobFunction = void(*)(struct Job*);

	struct alignas(64) Job
	{
		JobFunction fn{ nullptr };
		Job* parent{ nullptr };
		Job* continuation{ nullptr };
		std::atomic_int32_t unfinished_jobs{ 1 };
		uint32_t padding0{ 0 };

		static constexpr size_t kDataSize = 128 - 32;
		std::byte data[kDataSize];

		template <typename T, typename... Args>
		T* EmplaceParams(Args&&... args)
		{
			static_assert(alignof(T) <= 32);
			static_assert(sizeof(T) <= Job::kDataSize);
			return ::new (static_cast<void*>(data)) T(std::forward<Args>(args)...);
		}
	};

	static_assert(sizeof(Job) == 128);



}