#include "pch.h"
#include <sal.h>
#include <Windows.h>
#include <Engine/Asserts.h>

int RunGame(HINSTANCE hInstance, int nCmdShow);

namespace rr::test
{
	void TestJobSystem();
	bool TestTinyGLTFv3();
	void TestFileBatch();
	void TestStableArray();
	void TestStableArena();
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
	using namespace rr;
	rr::test::TestJobSystem();
	rr::test::TestFileBatch();
	rr::test::TestStableArray();
	rr::test::TestStableArena();
	//return RunGame(hInstance, nCmdShow);

	return 0;
}


