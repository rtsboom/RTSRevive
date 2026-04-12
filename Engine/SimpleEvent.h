#pragma once

#include <Windows.h>
#include <utility>
namespace rr
{
	class SimpleEvent
	{
	public:
		SimpleEvent() = default;
		explicit SimpleEvent(HANDLE handle) : m_handle(handle) {};
		~SimpleEvent() 
		{
			if (m_handle) ::CloseHandle(m_handle);
		}

		SimpleEvent(SimpleEvent const&) = delete;
		SimpleEvent& operator=(SimpleEvent const&) = delete;
		SimpleEvent(SimpleEvent&& other) noexcept
		{
			m_handle = std::exchange(other.m_handle,nullptr);
		}

		SimpleEvent& operator=(SimpleEvent&& other) noexcept
		{
			if (this != &other)
			{
				if (m_handle) ::CloseHandle(m_handle);
				m_handle = std::exchange(other.m_handle, nullptr);
			}
			return *this;
		}

	public:
		HANDLE Get() const { return m_handle; }
		bool IsValid() const { return m_handle != nullptr; }

	private:
		HANDLE m_handle = nullptr;
	};
}