#pragma once
#include <atomic>
#include <vector>
#include <cassert>
#include <cstdint>
namespace rr
{
	template <typename T>
	class SPMCQueue
	{
	public:
		SPMCQueue(uint64_t capacity)
			: capacity_{ capacity }
			, capacity_mask_{ capacity - 1 }
			, head_{ 0 }
			, tail_{ 0 }
		{
			assert((capacity & capacity_mask_) == 0); // capacity must be a power of two

			buffer_.resize(capacity);
		}

		bool Push(T const& value)
		{
			uint64_t const t = tail_.load(std::memory_order_relaxed);
			uint64_t const h = head_.load(std::memory_order_acquire);

			if ((t - h) == capacity_) return false; // full

			uint64_t const index = t & capacity_mask_;
			buffer_[index] = value;

			tail_.store(t + 1, std::memory_order_release);
			return true;
		}

		bool Pop(T& out)
		{
			uint64_t h = head_.load(std::memory_order_relaxed);

			for (;;)
			{
				uint64_t const t = tail_.load(std::memory_order_acquire);

				if ((t - h) == 0) return false; // empty

				uint64_t const index = h & capacity_mask_;
				T value = buffer_[index];

				if (head_.compare_exchange_weak(
					h, h + 1,
					std::memory_order_release,
					std::memory_order_relaxed))
				{
					out = value;
					return true;
				}
			}
		}

	private:
		std::vector<T> buffer_;
		uint64_t capacity_;
		uint64_t capacity_mask_;
		alignas(64) std::atomic_uint64_t head_;
		alignas(64) std::atomic_uint64_t tail_;
	};
}