#include "pch.h"
#include <sal.h>
#include <Windows.h>
#include <Engine/Asserts.h>

int RunGame(HINSTANCE hInstance, int nCmdShow);

namespace rr::test
{
	void RunJobSystemTests();
	bool TestTinyGLTFv3();
	void TestFileBatch();
	void TestStableArray();
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
	using namespace rr;
	//rr::test::RunJobSystemTests();
	//rr::test::TestFileBatch();
	rr::test::TestStableArray();

	//return RunGame(hInstance, nCmdShow);

	RR_ASSERT(true);
	RR_ASSERT_MSG(false, "hello faile assert");

	return 0;
}


