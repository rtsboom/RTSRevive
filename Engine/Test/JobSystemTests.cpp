#include "pch.h"
#include "Engine/JobSystem.h"
#include <cassert>

namespace
{
	using namespace rr;

	void CounterJob(std::atomic_int32_t* counter)
	{
		counter->fetch_add(1, std::memory_order_relaxed);
	}

	struct FanOutJobData
	{
		std::atomic_int32_t* counter{ nullptr };
		int depth{ 0 };
	};

	void FanOutJob(JobSystem& sys, JobID self, FanOutJobData& data)
	{
		data.counter->fetch_add(1, std::memory_order_relaxed);
		if (data.depth <= 0)
			return;

		JobID a = sys.CreateJobAsChild<FanOutJob>(self, FanOutJobData{ data.counter, data.depth - 1 });
		JobID b = sys.CreateJobAsChild<FanOutJob>(self, FanOutJobData{ data.counter, data.depth - 1 });

		assert(a != JobID_Null);
		assert(b != JobID_Null);

		sys.RunJob(a);
		sys.RunJob(b);
	}

	void BasicExecute()
	{
		JobSystem js;
		js.Initialize(4);
		std::atomic_int32_t counter{ 0 };
		JobID job = js.CreateJob<CounterJob>(&counter);

		assert(job != JobID_Null);

		js.RunJob(job);
		js.WaitJob(job);

		assert(counter.load(std::memory_order_relaxed) == 1);
		js.Shutdown();
	}

	void ManyIndependentJobs()
	{
		JobSystem js;
		js.Initialize(4);

		constexpr int job_count = 10'000;
		std::atomic<int> counter = 0;

		JobID root = js.CreateJob<CounterJob>(&counter);
		assert(root != JobID_Null);

		for (int i = 0; i < job_count; ++i)
		{
			JobID child = js.CreateJobAsChild<CounterJob>(root, &counter);
			assert(child != JobID_Null);
			js.RunJob(child);
		}

		js.RunJob(root);
		js.WaitJob(root);

		// Add 1 to the final count for the root job.
		assert(counter.load(std::memory_order_relaxed) == job_count + 1);

		js.Shutdown();
	}

	void FanOut()
	{
		JobSystem js;
		js.Initialize(4);

		std::atomic<int> counter = 0;

		constexpr int depth = 10;
		constexpr int expected = (1 << (depth + 1)) - 1;

		JobID root = js.CreateJob<FanOutJob>(FanOutJobData{ &counter, depth });
		assert(root != JobID_Null);

		js.RunJob(root);
		js.WaitJob(root);

		assert(counter.load(std::memory_order_relaxed) == expected);

		js.Shutdown();
	}

	void Stress()
	{
		for (int worker_count : { 1, 2, 4, 8 })
		{
			JobSystem js;
			js.Initialize(worker_count);

			constexpr int job_count = 30'000; // Max 2^15
			std::atomic<int> counter = 0;

			JobID root = js.CreateJob<CounterJob>(&counter);


			assert(root != JobID_Null);

			for (int i = 0; i < job_count; ++i)
			{
				JobID child = js.CreateJobAsChild<CounterJob>(root, &counter);

				assert(child != JobID_Null);
				js.RunJob(child);
			}

			js.RunJob(root);
			js.WaitJob(root);

			assert(counter.load(std::memory_order_relaxed) == job_count + 1);

			js.Shutdown();
		}
	}

	void FrameReset()
	{
		JobSystem js;
		js.Initialize(8);

		constexpr int job_count{ 1000 };
		constexpr int frame_count{ 1000 };
		for (int frame = 0; frame < frame_count; ++frame)
		{

			js.FrameReset();
			std::atomic<int> counter{ 0 };

			JobID root = js.CreateJob<CounterJob>(&counter);

			assert(root != JobID_Null);

			for (int i = 0; i < job_count; ++i)
			{
				JobID child = js.CreateJobAsChild<CounterJob>(root, &counter);

				assert(child != JobID_Null);
				js.RunJob(child);
			}

			js.RunJob(root);
			js.WaitJob(root);

			assert(counter.load(std::memory_order_relaxed) == job_count + 1);
		}

		assert(js.GetTotalExecutedJobs() == (job_count + 1) * frame_count);
		assert(js.GetTotalPushedJobs() == (job_count + 1) * frame_count);
		js.Shutdown();
	}

	void BasicContinuation()
	{
		JobSystem js;
		js.Initialize(4);

		std::atomic<int> counter{ 0 };

		auto a = js.CreateJob<CounterJob>(&counter);
		auto b = js.CreateJob<CounterJob>(&counter);


		js.SetContinuation(a, b);

		js.RunJob(a);
		js.WaitJob(b);

		assert(counter == 2);

		js.Shutdown();
	}

	void LongContinuation()
	{
		JobSystem js;
		js.Initialize(4);

		constexpr int continuation_count = 1000;
		std::atomic<int> counter{ 0 };

		auto const root = js.CreateJob<CounterJob>(&counter);

		auto prev = root;
		for (int i{}; i < continuation_count; ++i)
		{
			auto next = js.CreateJob<CounterJob>(&counter);
			js.SetContinuation(prev, next);
			prev = next;
		}

		js.RunJob(root);
		js.WaitJob(prev); // last one
		assert(counter == continuation_count + 1);
		js.Shutdown();
	}
}
namespace rr::test
{
	void RunJobSystemTests()
	{
		BasicExecute();
		ManyIndependentJobs();
		FanOut();
		Stress();
		FrameReset();
		BasicContinuation();
		LongContinuation();
	}
}