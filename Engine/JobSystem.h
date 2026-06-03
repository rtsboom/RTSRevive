#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
namespace rr
{
	constexpr uint16_t JobID_Null = UINT16_MAX;
	using JobID = uint16_t;
	using JobFn = void(*)(struct Job* self);
	struct alignas(64) Job
	{
		JobFn fn;
		void* context;
		void* data;
		JobID parent;
		JobID continuation;
		std::atomic_uint32_t unfinished_jobs;
		alignas(32) std::byte inline_data[32];
	};
	static_assert(sizeof(Job) == 64);
	static_assert(alignof(Job) == 64);



	namespace JobSystem
	{
		constexpr uint32_t kJobsPerWorker = 1024;
		constexpr uint32_t kJobQueueMask = kJobsPerWorker - 1;


		void Initialize(uint32_t worker_count);
		void Shutdown();

		void RunJob(JobID id);
	}
}