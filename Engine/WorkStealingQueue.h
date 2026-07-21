#pragma once
#include <type_traits>
#include <atomic>
#include <cstdint>
#include "Asserts.h"
#include <memory>

namespace rr
{
	template<typename T, uint64_t Capacity>
	class WorkStealingQueue
	{
		static constexpr uint64_t capacity_ = Capacity;
		static constexpr uint64_t capacity_mask_ = Capacity - 1;
		static_assert(capacity_ > 0 && (capacity_ & (capacity_mask_)) == 0, "Capacity must be a power of two");
		static_assert(std::is_trivially_copyable_v<T>);
		static_assert(std::atomic<T>::is_always_lock_free);

	public:
		WorkStealingQueue()
			: slots_{ std::make_unique<std::atomic<T>[]>(capacity_) }
		{
		}

		bool Push(T value)
		{
			uint64_t const b = bottom_.load(std::memory_order_relaxed);

			// Must be acquire to syncronize-with the successful top CAS in Steal().
			// Ensures the thief has completed reading the slot before it is reused.
			uint64_t const t = top_.load(std::memory_order_acquire); 

			uint64_t const size = b - t;
			if (size >= capacity_) return false; // full

			uint64_t const index = b & capacity_mask_;
			slots_[index].store(value, std::memory_order_relaxed);

			bottom_.store(b + 1, std::memory_order_release);

			return true;
		}

		bool Pop(T& out)
		{
			uint64_t const b = bottom_.load(std::memory_order_relaxed);
			bottom_.store(b - 1, std::memory_order_release);

			std::atomic_thread_fence(std::memory_order_seq_cst);

			uint64_t       t = top_.load(std::memory_order_relaxed);
			uint64_t const size = b - t;

			RR_ASSERT(size <= capacity_); // (size > capacity_) is not possible.
			if (size == 0) // The queue is empty.
			{
				bottom_.store(b, std::memory_order_release);
				return false;
			}


			uint64_t const index = (b - 1) & capacity_mask_;
			T value = slots_[index].load(std::memory_order_relaxed);

			if (size == 1) // last item, must contend with thieves
			{
				bool result{ false };
				if (top_.compare_exchange_strong(
					t, t + 1,
					std::memory_order_seq_cst,
					std::memory_order_relaxed))
				{
					out = value;
					result = true;
				}

				bottom_.store(b, std::memory_order_release);
				return result;
			}

			out = value;
			return true;
		}

		bool Steal(T& out)
		{
			uint64_t       t = top_.load(std::memory_order_relaxed);
			uint64_t const b = bottom_.load(std::memory_order_acquire); // get published bottom value

			uint64_t const size = b - t;
			if (size == 0 || size > capacity_) // The queue is empty.
				return false;


			// Read the value before CAS.
			// After a successful CAS, this slot may be immediately resued by another thread.
			uint64_t const index = t & capacity_mask_;
			T value = slots_[index].load(std::memory_order_relaxed);

			bool result{ false };
			if (top_.compare_exchange_strong(
				t, t + 1,
				std::memory_order_seq_cst,
				std::memory_order_relaxed))
			{
				out = value;
				result = true;
			}

			return result;
		}

		// Only call from the owner thread
		bool IsEmpty() const noexcept
		{
			uint64_t const b = bottom_.load(std::memory_order_relaxed);
			uint64_t const t = top_.load(std::memory_order_acquire);
			return (b - t) == 0;
		}

	private:
		alignas(64) std::atomic_uint64_t top_{ 0 }; // thief side
		alignas(64) std::atomic_uint64_t bottom_{ 0 }; // owner side
		alignas(64) std::unique_ptr<std::atomic<T>[]> slots_;
	};
}

