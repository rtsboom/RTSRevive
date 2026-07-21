#include "pch.h"
#include "Engine/JobSystem.h"
#include <cassert>

namespace
{
	using namespace rr;

	void CounterJob(Job*, std::atomic_int32_t* counter)
	{
		counter->fetch_add(1, std::memory_order_relaxed);
	}

	struct FanOutJobData
	{
		std::atomic_int32_t* counter{ nullptr };
		int depth{ 0 };
	};

	void FanOutJob(Job* self, FanOutJobData& data)
	{
		data.counter->fetch_add(1, std::memory_order_relaxed);
		if (data.depth <= 0)
			return;

		Job* a = self->CreateChild<FanOutJob>(FanOutJobData{ data.counter, data.depth - 1 });
		Job* b = self->CreateChild<FanOutJob>(FanOutJobData{ data.counter, data.depth - 1 });

		RR_ASSERT(a != nullptr);
		RR_ASSERT(b != nullptr);

		self->Run(a);
		self->Run(b);
	}

	void BasicExecute()
	{
		JobSystem js;
		js.Initialize(4);
		std::atomic_int32_t counter{ 0 };
		Job* job = js.CreateJob<CounterJob>(&counter);

		RR_ASSERT(job != nullptr);

		js.RunJob(job);
		js.WaitJob(job);

		RR_ASSERT(counter.load(std::memory_order_relaxed) == 1);
		js.Shutdown();
	}

	void ManyIndependentJobs()
	{
		JobSystem js;
		js.Initialize(4);

		constexpr int job_count = 1023;
		std::atomic<int> counter = 0;

		Job* root = js.CreateJob<CounterJob>(&counter);
		RR_ASSERT(root != nullptr);

		for (int i = 0; i < job_count; ++i)
		{
			Job* child = js.CreateJobAsChild<CounterJob>(root, &counter);
			RR_ASSERT(child != nullptr);
			js.RunJob(child);
		}

		js.RunJob(root);
		js.WaitJob(root);

		// Add 1 to the final count for the root job.
		RR_ASSERT(counter.load(std::memory_order_relaxed) == job_count + 1);

		js.Shutdown();
	}

	void FanOut()
	{
		JobSystem js;
		js.Initialize(4);

		std::atomic<int> counter = 0;

		constexpr int depth = 10;
		constexpr int expected = (1 << (depth + 1)) - 1;

		Job* root = js.CreateJob<FanOutJob>(FanOutJobData{ &counter, depth });
		RR_ASSERT(root != nullptr);

		js.RunJob(root);
		js.WaitJob(root);

		RR_ASSERT(counter.load(std::memory_order_relaxed) == expected);

		js.Shutdown();
	}

	void Stress()
	{
		for (int worker_count : { 1, 2, 4, 8 })
		{
			JobSystem js;
			js.Initialize(worker_count);

			constexpr int job_count = 1023;
			std::atomic<int> counter = 0;

			Job* root = js.CreateJob<CounterJob>(&counter);


			RR_ASSERT(root != nullptr);

			for (int i = 0; i < job_count; ++i)
			{
				Job* child = js.CreateJobAsChild<CounterJob>(root, &counter);

				RR_ASSERT(child != nullptr);
				js.RunJob(child);
			}

			js.RunJob(root);
			js.WaitJob(root);

			RR_ASSERT(counter.load(std::memory_order_relaxed) == job_count + 1);

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
			std::atomic<int> counter{ 0 };

			Job* root = js.CreateJob<CounterJob>(&counter);

			RR_ASSERT(root != nullptr);

			for (int i = 0; i < job_count; ++i)
			{
				Job* child = js.CreateJobAsChild<CounterJob>(root, &counter);

				RR_ASSERT(child != nullptr);
				js.RunJob(child);
			}

			js.RunJob(root);
			js.WaitJob(root);

			RR_ASSERT(counter.load(std::memory_order_relaxed) == job_count + 1);
			js.Reset();
		}

		constexpr size_t total_expected = (job_count + 1) * frame_count;
		RR_ASSERT(js.GetTotalExecutedJobs() == total_expected);
		RR_ASSERT(js.GetTotalSubmittedJobs() == total_expected);
		js.Shutdown();
	}

	void BasicContinuation()
	{
		JobSystem js;
		js.Initialize(4);

		std::atomic<int> counter{ 0 };

		auto a = js.CreateJob<CounterJob>(&counter);
		auto b = js.CreateJob<CounterJob>(&counter);

		a->SetNext(b);

		js.RunJob(a);
		js.WaitJob(b);

		RR_ASSERT(counter == 2);

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
			prev->SetNext(next);
			prev = next;
		}

		js.RunJob(root);
		js.WaitJob(prev); // last one

		RR_ASSERT(counter == continuation_count + 1);
		js.Shutdown();
	}
}
namespace rr::test
{
	void TestJobSystem()
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