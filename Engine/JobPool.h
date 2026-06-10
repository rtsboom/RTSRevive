#pragma once
#include "Job.h"

#include <vector>
#include <atomic>
#include <cstdint>
#include "SPMCQueue.h"


namespace rr
{
	struct alignas(64) JobChunk
	{
		std::atomic_uint32_t generation; // memory_order_relaxed is sufficient
		std::atomic_uint32_t free_count;
	};

	class JobPool
	{
		static constexpr size_t jobs_per_chunk_ = 128;
		static constexpr size_t total_chunks_ = 256;
	public:
		JobPool();

		JobChunk* AllocateChunk();
		Job* GetJob(JobID job_idx) noexcept { return &jobs_[job_idx]; }

	private:
		std::vector<Job> jobs_;
		std::vector<JobChunk> chunks_;

		SPMCQueue<uint32_t, total_chunks_> free_chunks;
	};
}

