#pragma once
#include <type_traits>
#include <atomic>
#include <memory>
#include <cstdint>

namespace rr
{
	template<typename T, uint64_t Capacity>
	class WorkStealingQueue
	{
		static constexpr uint64_t capacity_ = Capacity;
		static constexpr uint64_t capacity_mask_ = Capacity - 1;
		static_assert(capacity_ > 0 && (capacity_ & (capacity_mask_)) == 0, "Capacity must be a power of two");
		static_assert(std::is_trivially_copyable_v<T>);
	public:
		WorkStealingQueue()
			: top_{ 0 }
			, bottom_{ 0 }
		{
			buffer_ = std::make_unique<T[]>(capacity_);
		}

		bool Push(T value)
		{
			uint64_t const b = bottom_.load(std::memory_order_relaxed);
			uint64_t const t = top_.load(std::memory_order_acquire);

			uint64_t const size = b - t;
			if (size >= capacity_) return false; // full

			uint64_t const index = b & capacity_mask_;
			buffer_[index] = value;

			bottom_.store(b + 1, std::memory_order_release);

			return true;
		}

		bool Pop(T& out)
		{
			uint64_t const b = bottom_.load(std::memory_order_relaxed);
			bottom_.store(b - 1, std::memory_order_relaxed);

			std::atomic_thread_fence(std::memory_order_seq_cst);

			uint64_t       t = top_.load(std::memory_order_relaxed);
			uint64_t const size = b - t;
			if (size == 0 || size > capacity_) // empty 
			{
				bottom_.store(b, std::memory_order_relaxed);
				return false;
			}

			uint64_t const index = (b - 1) & capacity_mask_;
			T value = buffer_[index];

			if (size == 1) // last item, must contend with stealers
			{
				bool result{ false };
				if (top_.compare_exchange_strong(
					t, t + 1,
					std::memory_order_acq_rel,
					std::memory_order_relaxed))
				{
					out = value;
					result = true;
				}

				bottom_.store(b, std::memory_order_relaxed);
				return result;
			}

			out = value;
			return true;
		}

		bool Steal(T& out)
		{
			uint64_t       t = top_.load(std::memory_order_acquire);
			uint64_t const b = bottom_.load(std::memory_order_acquire);

			uint64_t const size = b - t;
			if (size == 0 || size > capacity_) return false; // empty


			uint64_t const index = t & capacity_mask_;
			T value = buffer_[index];
			bool result{ false };

			if (top_.compare_exchange_strong(
				t, t + 1,
				std::memory_order_acq_rel,
				std::memory_order_relaxed))
			{
				out = value;
				result = true;
			}

			return result;
		}

	private:
		alignas(64) std::atomic_uint64_t top_; // steal side
		alignas(64) std::atomic_uint64_t bottom_; // owner side
		alignas(64) std::unique_ptr<T[]> buffer_;
	};
}

