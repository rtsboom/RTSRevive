#include "pch.h"
#include <sal.h>
#include <Windows.h>
#include <Engine/VirtualArena.h>

int RunGame(HINSTANCE hInstance, int nCmdShow);

namespace rr::test
{
	void RunJobSystemTests();
}



int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
	using namespace rr;
	//rr::test::RunJobSystemTests();

	//return RunGame(hInstance, nCmdShow);
	return 0;
}


