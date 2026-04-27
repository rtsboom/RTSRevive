#include "pch.h"
#include "Application.h"

namespace rr
{
	Application::Application(HWND hwnd, uint32_t window_width, uint32_t window_height)
		: m_hwnd(hwnd)
		, m_window_width(window_width)
		, m_window_height(window_height)
	{
		m_gfx_core = GpuDevice(m_hwnd);

	}
	void Application::OnWindowResize(uint32_t width, uint32_t height)
	{
		m_gfx_core.Resize(width, height);
	}
}