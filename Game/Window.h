#pragma once

#include "InputManager.h"
class Window
{
public:
	bool Init(int width, int height, const wchar_t* title);
	void Shutdown();
	bool ShouldClose() const { return m_shouldClose; }
	HWND GetHandle() const { return m_hwnd; }



	InputManager& GetInput() { return m_input; }
private:
	HWND m_hwnd = nullptr;
	bool m_shouldClose = false;

	InputManager m_input;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
};