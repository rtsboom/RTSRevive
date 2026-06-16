#include "pch.h"
#include "Engine/JobSystem.h"

namespace rr::test
{
	void RunJobSystemTests()
	{
		size_t physical_cores = std::thread::hardware_concurrency() / 2;
		JobSystem js;
		js.Initialize(physical_cores);
		js.Shutdown();
	}
}