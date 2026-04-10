#pragma once
#include <chrono>

class Timer
{
public:
	void Reset() { m_prev = std::chrono::steady_clock::now(); }
	void Tick()
	{
		auto now = std::chrono::steady_clock::now();
		m_dt = std::chrono::duration<float>(now - m_prev).count();
		m_prev = now;
	}
	float GetDeltaTime() const { return m_dt; }

private:
	std::chrono::steady_clock::time_point m_prev;
	float m_dt = 0.0f;
};