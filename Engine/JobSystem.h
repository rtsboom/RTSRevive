#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
namespace rr
{
	using JobID = uint16_t;
	constexpr JobID JobID_Null = UINT16_MAX;
	using JobFn = void(*)(JobID id, void* data);
	struct alignas(64) Job
	{
		JobFn fn;

		void* data;
		JobID parent;
		JobID continuation;
		std::atomic_uint32_t unfinished_jobs;
		alignas(8) std::byte inline_data[40];
	};
	static_assert(sizeof(Job) == 64);
	static_assert(alignof(Job) == 64);



	namespace JobSystem
	{
		constexpr uint64_t kJobsPerWorker = 1024;
		constexpr uint64_t kJobQueueMask = kJobsPerWorker - 1;
		static_assert((kJobsPerWorker & (kJobsPerWorker - 1)) == 0, "kJobsPerWorker must be a power of two");



		void Initialize(uint32_t worker_count);
		void Shutdown();

		void RunJob(JobID id);
	}
}