#include "EnginePch.h"
#include "Job.h"
#include "JobPool.h"

namespace rr
{
	JobPool::JobPool()
	{
		size_t const total_jobs = jobs_per_chunk_ * total_chunks_;
		jobs_.resize(total_jobs);
		chunks_.resize(total_chunks_);
	}

	JobChunk* JobPool::AllocateChunk()
	{


		return nullptr;
	}


}