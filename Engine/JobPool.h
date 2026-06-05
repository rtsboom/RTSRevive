#pragma once
#include <cstdint>

namespace rr
{
	struct Job;

	class JobPool
	{
	public:
		Job* Allocate();
		void Free(uint32_t job_idx);
		Job* GetJob(uint32_t job_idx);
	};
}

