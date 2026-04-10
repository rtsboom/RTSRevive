#include "pch.h"
#include "App.h"
#include <objbase.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr)) return false;

	using namespace rrv;
	App app;
	if (!app.Init())
		return -1;

	app.Run();
	app.Shutdown();

	return 0;
}