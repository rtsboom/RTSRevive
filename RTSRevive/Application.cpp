#include "pch.h"
#include "Application.h"

namespace rr
{
	Application::Application(HWND hwnd, uint32_t window_width, uint32_t window_height)
		: hwnd_(hwnd)
		, window_width_(window_width)
		, window_height_(window_height)
	{
		gpu_device = GPUDevice(hwnd_);

	}
	void Application::OnWindowResize(uint32_t width, uint32_t height)
	{
		gpu_device.Resize(width, height);
	}
}