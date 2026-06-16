#include "pch.h"
#include <Windows.h>

#include "Application.h"
#include <combaseapi.h>
#include <DirectXMath.h>
#include <memory>
#include <cstdint>


std::unique_ptr<rr::Application> g_app;
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	using namespace rr;

	switch (msg)
	{
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostQuitMessage(0);
			return 0;
		}
		break;

	case WM_SIZE:
	{
		if (wparam == SIZE_MINIMIZED) break;
		const uint32_t width = LOWORD(lparam);
		const uint32_t height = HIWORD(lparam);
		if (width == 0 || height == 0) break;
		if (g_app) g_app->OnWindowResize(width, height);
	}
	break;

	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int RunGame(HINSTANCE hInstance, int nCmdShow)
{
	if (!DirectX::XMVerifyCPUSupport())return 1;
	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) return 1;

	SetProcessDPIAware();

	RECT rc = { 0, 0, 1280, 720 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	int const w = rc.right - rc.left;
	int const h = rc.bottom - rc.top;
	int const x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
	int const y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;



	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(nullptr, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszClassName = L"RTSRevive";
	if (!RegisterClassExW(&wc)) return 1;

	HWND h_window = CreateWindowExW(0, wc.lpszClassName, wc.lpszClassName, WS_OVERLAPPEDWINDOW,
		x, y, w, h,
		nullptr, nullptr, wc.hInstance, nullptr);

	if (!h_window) return 1;
	ShowWindow(h_window, SW_SHOW);
	GetClientRect(h_window, &rc); // Get the actual client area size
	g_app = std::make_unique<rr::Application>(h_window, rc.right - rc.left, rc.bottom - rc.top);

	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			g_app->Tick();
		}
	}

	g_app.reset();
	DestroyWindow(h_window);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);
	CoUninitialize();

	return static_cast<int>(msg.wParam);
}

