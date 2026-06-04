#pragma once
#include "Job.h"

#include <atomic>
#include <vector>
namespace rr
{
	class WorkStealingQueue
	{
	public:
		WorkStealingQueue(size_t capacity);
		void Push(Job* job);
		Job* Pop();
		Job* Steal();

	private:
		alignas(64) std::atomic_int64_t top_{ 0 };
		alignas(64) std::atomic_int64_t bottom_{ 0 };
		alignas(64) std::vector<Job*> buffer_;
		size_t capacity_mask_;
	};
}

