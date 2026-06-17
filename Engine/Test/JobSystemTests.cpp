#include "pch.h"
#include "Engine/JobSystem.h"

namespace rr::test
{
	namespace
	{

		struct CounterJobData
		{
			std::atomic_int32_t* counter{ nullptr };
		};

		void CounterJob(JobSystem& sys, JobID self, CounterJobData& data)
		{
			data.counter->fetch_add(1, std::memory_order_relaxed);
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

			JobID a = sys.CreateJobAsChild<FanOutJobData, FanOutJob>(self, data.counter, data.depth - 1);
			JobID b = sys.CreateJobAsChild<FanOutJobData, FanOutJob>(self, data.counter, data.depth - 1);
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
			JobID job = js.CreateJob<CounterJobData, CounterJob>(&counter);
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

			JobID root = js.CreateJob<CounterJobData, CounterJob>(&counter);
			assert(root != JobID_Null);

			for (int i = 0; i < job_count; ++i)
			{
				JobID child = js.CreateJobAsChild<CounterJobData, CounterJob>(root, &counter);
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

			JobID root = js.CreateJob<FanOutJobData, FanOutJob>(&counter, depth);
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

				JobID root = js.CreateJob<CounterJobData, CounterJob>(&counter);
				assert(root != JobID_Null);

				for (int i = 0; i < job_count; ++i)
				{
					JobID child = js.CreateJobAsChild<CounterJobData, CounterJob>(root, &counter);
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
			js.Initialize(4);

			for (int frame = 0; frame < 100; ++frame)
			{
				js.BeginFrame();
				std::atomic<int> counter = 0;

				constexpr int job_count = 1000;

				JobID root = js.CreateJob<CounterJobData, CounterJob>(&counter);
				assert(root != JobID_Null);

				for (int i = 0; i < job_count; ++i)
				{
					JobID child =
						js.CreateJobAsChild<CounterJobData, CounterJob>(
							root,
							&counter);

					assert(child != JobID_Null);
					js.RunJob(child);
				}

				js.RunJob(root);
				js.WaitJob(root);

				assert(counter.load(std::memory_order_relaxed) == job_count + 1);
			}

			js.Shutdown();
		}
	}

	void RunJobSystemTests()
	{
		BasicExecute();
		ManyIndependentJobs();
		FanOut();
		Stress();
		FrameReset();
	}
}