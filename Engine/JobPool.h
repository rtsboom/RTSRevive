#pragma once
#include "Job.h"

#include <vector>
#include <atomic>
#include <cstdint>
#include <limits>
#include <cassert>

namespace rr
{
	struct FrameJobCursor
	{
		uint32_t chunk_index = UINT32_MAX;
		uint32_t job_offset_in_chunk = UINT32_MAX;
	};

	class FrameJobPool
	{
		static constexpr uint32_t total_chunks_{ 512 };
		static constexpr uint32_t total_chunks_mask_{ total_chunks_ - 1 };
		static constexpr uint32_t jobs_per_chunk_{ 64 };
		static constexpr uint32_t total_jobs_{ jobs_per_chunk_ * total_chunks_ };

		static_assert((jobs_per_chunk_& (jobs_per_chunk_ - 1)) == 0, "jobs per chunk must be a power of two");
		static_assert((total_chunks_& (total_chunks_ - 1)) == 0, "total chunks must be a power of two");
		static_assert(total_jobs_ <= (1u << 15));
	public:
		FrameJobPool()
		{
			jobs_.resize(total_jobs_);
		}

		void Reset()
		{
			uint32_t const used_chunks = chunk_offset_.exchange(0, std::memory_order_relaxed);
			assert(used_chunks <= total_chunks_);

			frame_chunk_begin_ += used_chunks;
		}

		JobID Allocate(FrameJobCursor& cursor)
		{
			if (cursor.chunk_index >= total_chunks_
				|| cursor.job_offset_in_chunk >= jobs_per_chunk_)
			{
				uint32_t const offset = chunk_offset_.fetch_add(1, std::memory_order_relaxed);
				if (offset >= total_chunks_)
				{
					return std::numeric_limits<JobID>::max();
				}

				cursor.chunk_index = (frame_chunk_begin_ + offset) & total_chunks_mask_;
				cursor.job_offset_in_chunk = 0;
			}
			uint32_t const job_index =
				cursor.chunk_index * jobs_per_chunk_
				+ cursor.job_offset_in_chunk++;

			return job_index;
		}
		Job* GetJob(JobID job_index) noexcept 
		{
			assert(job_index < jobs_.size());
			return &jobs_[job_index]; 
		}

	private:
		std::vector<Job> jobs_;
		uint32_t frame_chunk_begin_{ 0 };
		std::atomic_uint32_t chunk_offset_{ 0 };
	};
}

