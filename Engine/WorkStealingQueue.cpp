#include "EnginePch.h"
#include "WorkStealingQueue.h"
#include "Job.h"

#include <atomic>
#include <cassert>
#include <cstdint>

namespace rr
{
	WorkStealingQueue::WorkStealingQueue(size_t capacity)
	{
		assert((capacity & (capacity - 1)) == 0); // capacity must be a power of two
		buffer_.resize(capacity);
		capacity_mask_ = capacity - 1;
	}
	void WorkStealingQueue::Push(Job* job)
	{
		int64_t b = bottom_.load(std::memory_order_relaxed);
		buffer_[b] = job;

		bottom_.store(++b, std::memory_order_release);
	}
	Job* WorkStealingQueue::Pop()
	{
		int64_t b = bottom_.load(std::memory_order_relaxed);
		bottom_.store(--b, std::memory_order_relaxed);

		std::atomic_thread_fence(std::memory_order_seq_cst);
		int64_t t = top_.load(std::memory_order_relaxed);

		if (t <= b)
		{
			Job* job = buffer_[b & capacity_mask_];
			if (t == b)
			{
				if (!top_.compare_exchange_strong(
					t, t + 1,
					std::memory_order_acq_rel, 
					std::memory_order_relaxed))
				{
					job = nullptr;
				}
				bottom_.store(b + 1, std::memory_order_relaxed);
			}

			return job;
		}

		bottom_.store(b + 1, std::memory_order_relaxed);
		return nullptr;
	}
	Job* WorkStealingQueue::Steal()
	{
		int64_t t = top_.load(std::memory_order_acquire);
		int64_t b = bottom_.load(std::memory_order_acquire);

		if (t < b) // empty queue
		{
			Job* job = buffer_[t & capacity_mask_];

			if (top_.compare_exchange_strong(
				t, t + 1,
				std::memory_order_acq_rel,
				std::memory_order_relaxed))
			{
				return job;
			}
		}

		return nullptr;
	}
}