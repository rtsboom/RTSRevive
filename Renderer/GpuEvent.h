#pragma once
#include <Windows.h>
#include <utility>
#include <stdexcept>
namespace rrv
{
	// wrapper of HANDLE, used for GPU events such as fence completion
	class GpuEvent
	{
	public:
		~GpuEvent()
		{
			if (m_event) CloseHandle(m_event);
		}
		GpuEvent() = default;
		GpuEvent(GpuEvent&& other) noexcept : GpuEvent()
		{
			swap(*this, other);
		}
		GpuEvent& operator=(GpuEvent&& other) noexcept
		{
			GpuEvent temp(std::move(other));
			swap(*this, temp);
			return *this;
		}
		GpuEvent(const GpuEvent&) = delete;
		GpuEvent& operator=(const GpuEvent&) = delete;
	public:
		void Swap(GpuEvent& other) noexcept { std::swap(m_event, other.m_event); }
		friend void swap(GpuEvent& a, GpuEvent& b) noexcept { a.Swap(b); }
	public:
		explicit GpuEvent(HANDLE event) : m_event(event) { if (!m_event)throw std::runtime_error("CreateEvent failed"); }

	public:
		HANDLE Get() const { return m_event; }

	private:
		HANDLE m_event = nullptr;
	};

}
