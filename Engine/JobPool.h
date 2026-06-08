#pragma once
#include "Job.h"

#include <vector>
#include <atomic>
#include <cstdint>


namespace rr
{
	struct alignas(64) JobChunk
	{
		std::atomic_uint32_t generation; // memory_order_relaxed is sufficient
		std::atomic_uint32_t free_count;
	};

	class JobPool
	{
	public:
		JobPool(size_t jobs_per_chunk, size_t total_chunks);

		JobChunk* AllocateChunk();
		Job* GetJob(JobID job_idx) noexcept { return &jobs_[job_idx]; }

	private:
		size_t jobs_per_chunk_;
		size_t total_chunks_;
		std::atomic_uint32_t free_chunk_idx;



		std::vector<Job> jobs_;
		std::vector<JobChunk> chunks_;
	};
}

