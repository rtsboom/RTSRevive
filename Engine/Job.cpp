#include "pch.h"
#include "Job.h"
#include "JobSystem.h"
#include <Engine/StableArena.h>
#include <Engine/Asserts.h>

namespace rr
{
	void Job::Run(Job* job)
	{
		system_->RunJob(job);
	}

	void Job::SetNext(Job* job)
	{
		RR_ASSERT(this->next_ == nullptr);
		RR_ASSERT(job != nullptr);

		next_ = job;
	}

	StableArena& Job::GetScratchArena()
	{
		return system_->GetScratchArena();
	}
}
