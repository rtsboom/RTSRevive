#include "pch.h"
#include <sal.h>
#include <Windows.h>
#include <Engine/StableArray.h>


int RunGame(HINSTANCE hInstance, int nCmdShow);

namespace rr::test
{
	void RunJobSystemTests();
	bool TestTinyGLTFv3();
	void TestFileBatch();
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
	using namespace rr;
	//rr::test::RunJobSystemTests();
	//rr::test::TestFileBatch();
	StableArray<int> arr(10);


	//return RunGame(hInstance, nCmdShow);
	return 0;
}


