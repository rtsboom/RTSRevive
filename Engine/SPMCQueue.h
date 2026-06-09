#pragma once
#include <type_traits>
#include <atomic>
#include <vector>
#include <cstdint>
namespace rr
{
	template <typename T, uint64_t Capacity>
	class SPMCQueue
	{
		static constexpr uint64_t capacity_ = Capacity;
		static constexpr uint64_t capacity_mask_ = Capacity - 1;
		static_assert(capacity_ > 0 && (capacity_ & capacity_mask_) == 0, "Capacity must be a power of two");
		static_assert(std::is_trivially_copyable_v<T>);

	public:
		SPMCQueue()
			: head_{ 0 }
			, tail_{ 0 }
		{
			buffer_.resize(capacity_);
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
		alignas(64) std::atomic_uint64_t head_;
		alignas(64) std::atomic_uint64_t tail_;
	};
}