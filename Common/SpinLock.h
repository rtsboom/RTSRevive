#pragma once


#include <atomic>
#include <thread>
#include <immintrin.h> // _mm_pause

namespace rrv
{

	class SpinLock
	{
	public:
		void lock()
		{
			for (int i = 0; i < 16; ++i)
			{
				if (try_lock()) return;
				_mm_pause();
			}
			while (!try_lock())
				std::this_thread::yield();
		}

		void unlock()
		{
			m_flag.clear(std::memory_order_release);
		}

		bool try_lock()
		{
			return !m_flag.test_and_set(std::memory_order_acquire);
		}

	private:
		std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
	};
}
