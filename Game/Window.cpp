#include "pch.h"
#include "Window.h"
bool Window::Init(int width, int height, const wchar_t* title)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = L"RTSEngineWindow";
	RegisterClassEx(&wc);

	m_hwnd = CreateWindowEx(
		0, L"RTSEngineWindow", title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		nullptr, nullptr, wc.hInstance, this
	);

	if (!m_hwnd) return false;

	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);
	return true;
}

void Window::Shutdown()
{
	if (m_hwnd)
	{
		DestroyWindow(m_hwnd);
		m_hwnd = nullptr;
	}
}


LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	Window* self = nullptr;

	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
		self = reinterpret_cast<Window*>(cs->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	}
	else
	{
		self = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (self)
	{
		switch (msg)
		{
		case WM_DESTROY:
			self->m_shouldClose = true;
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			self->GetInput().OnKeyDown(static_cast<int>(wp));
			if (wp == VK_ESCAPE)
				self->m_shouldClose = true;
			return 0;
		case WM_KEYUP:
			self->GetInput().OnKeyUp(static_cast<int>(wp));
			return 0;
		case WM_LBUTTONDOWN:
			self->GetInput().OnMouseButtonDown(0);
			return 0;
		case WM_LBUTTONUP:
			self->GetInput().OnMouseButtonUp(0);
			return 0;
		case WM_RBUTTONDOWN:
			self->GetInput().OnMouseButtonDown(1);
			return 0;
		case WM_RBUTTONUP:
			self->GetInput().OnMouseButtonUp(1);
			return 0;
		case WM_MBUTTONDOWN:
			self->GetInput().OnMouseButtonDown(2);
			return 0;
		case WM_MBUTTONUP:
			self->GetInput().OnMouseButtonUp(2);
			return 0;
		case WM_MOUSEMOVE:
			self->GetInput().OnMouseMove(LOWORD(lp), HIWORD(lp));
			return 0;
		case WM_MOUSEWHEEL:
			self->GetInput().OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wp));
			return 0;
		}
	}

	return DefWindowProc(hwnd, msg, wp, lp);
}