#pragma once
#include <chrono>
#include <ratio>

namespace rr
{
	class Timer
	{
	public:
		using Clock = std::chrono::steady_clock;

		void Reset()
		{
			start_ = Clock::now();
		}

		double ElapsedSeconds() const
		{
			return std::chrono::duration<double>(Clock::now() - start_).count();
		}

		double ElapsedMilliseconds() const
		{
			return std::chrono::duration<double, std::milli>(Clock::now() - start_).count();
		}

	private:
		Clock::time_point start_ = Clock::now();
	};


}