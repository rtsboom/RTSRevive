#pragma once
#include <Engine/GPUDevice.h>

#include <Windows.h>
#include <cstdint>

namespace rr
{
	class Application
	{
	public:
		Application(HWND hwnd, uint32_t window_width, uint32_t window_height);
		void Tick() {};
		void OnWindowResize(uint32_t width, uint32_t height);
		
	private:
		HWND hwnd_;
		uint32_t window_width_;
		uint32_t window_height_;

		GPUDevice gpu_device;
	};

}