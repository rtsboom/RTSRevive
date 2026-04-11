#pragma once
#include <Windows.h>
#include <cstdint>

namespace rrv
{
	class Application
	{
	public:
		Application(HWND h_window, uint32_t window_width, uint32_t window_height);
		void Tick() {};

	private:
		HWND m_window_handle;
		uint32_t m_window_width;
		uint32_t m_window_height;
	};

}