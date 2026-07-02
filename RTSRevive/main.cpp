#include "pch.h"
#include <sal.h>
#include <Windows.h>
#include <Engine/DynamicArray.h>


int RunGame(HINSTANCE hInstance, int nCmdShow);

namespace rr::test
{
	void RunJobSystemTests();
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
	using namespace rr;
	//rr::test::RunJobSystemTests();

	DynamicArray<int> darr{ 100000 };

	darr.Resize(2000);
	darr.Resize(1000);
	darr.Resize(2000);

	darr.PushBack(1);
	darr.PushBack(2);
	darr.PushBack(3);
	darr.PushBack(4);
	darr.EmplaceBack(1);
	darr.EmplaceBack(2);
	darr.EmplaceBack(3);
	darr.EmplaceBack(4);

	auto r1 = darr.Back();
	darr.PopBack();
	auto r2 = darr.Back();
	darr.PopBack();
	auto r3 = darr.Back();
	darr.PopBack();
	auto r4 = darr.Back();
	darr.PopBack();
	darr.Clear();
	darr.ShrinkToFit();
	//return RunGame(hInstance, nCmdShow);
	return 0;
}


