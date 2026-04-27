#pragma once
#include <Engine/GpuDevice.h>

#include <Windows.h>
#include <cstdint>

namespace rr
{
	class Application
	{
	public:
		Application(HWND h_window, uint32_t window_width, uint32_t window_height);
		void Tick() {};
		void OnWindowResize(uint32_t width, uint32_t height);

	private:
		HWND m_hwnd;
		uint32_t m_window_width;
		uint32_t m_window_height;

		GpuDevice m_gfx_core;
	};

}