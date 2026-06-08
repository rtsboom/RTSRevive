#include "EnginePch.h"
#include "Job.h"
#include "JobPool.h"

namespace rr
{
	JobPool::JobPool(size_t jobs_per_chunk, size_t total_chunks)
		: jobs_per_chunk_{ jobs_per_chunk }
		, total_chunks_{ total_chunks }
	{
		size_t const total_jobs = jobs_per_chunk * total_chunks;
		jobs_.resize(total_jobs);
		chunks_.resize(total_chunks);
	}

	JobChunk* JobPool::AllocateChunk()
	{


		return nullptr;
	}


}