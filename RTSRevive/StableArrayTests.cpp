#include "pch.h"
#include <Engine/StableArray.h>
#include <Engine/Asserts.h>
namespace
{
	using namespace rr;

	struct Tracked
	{
		Tracked() { ++live_count; }
		explicit Tracked(int v) : value(v) { ++live_count; }
		Tracked(Tracked const& other) : value(other.value) { ++live_count; }
		Tracked(Tracked&& other) noexcept : value(other.value) { ++live_count; other.value = -1; }
		Tracked& operator=(Tracked const& other) { value = other.value; return *this; }
		Tracked& operator=(Tracked&& other) noexcept { value = other.value; other.value = -1; return *this; }
		~Tracked() { --live_count; }

		int value{ 0 };
		static inline int live_count{ 0 };
	};

	void TestStableArray_PushPopBasic()
	{
		StableArray<int> arr(16);

		RR_CHECK(arr.Size() == 0);

		arr.PushBack(1);
		arr.PushBack(2);
		arr.PushBack(3);

		RR_CHECK(arr.Size() == 3);
		RR_CHECK(arr[0] == 1 && arr[1] == 2 && arr[2] == 3);
		RR_CHECK(arr.Front() == 1);
		RR_CHECK(arr.Back() == 3);

		arr.PopBack();
		RR_CHECK(arr.Size() == 2);
		RR_CHECK(arr.Back() == 2);
	}

	void TestStableArray_DestructorCounts()
	{
		RR_CHECK(Tracked::live_count == 0);

		{
			StableArray<Tracked> arr(8);
			arr.EmplaceBack(1);
			arr.EmplaceBack(2);
			arr.EmplaceBack(3);

			RR_CHECK(Tracked::live_count == 3);

			arr.PopBack();
			RR_CHECK(Tracked::live_count == 2);

			arr.Clear();
			RR_CHECK(Tracked::live_count == 0);

			arr.EmplaceBack(10);
		}

		RR_CHECK(Tracked::live_count == 0);
	}

	void TestStableArray_ResizeDefault()
	{
		RR_CHECK(Tracked::live_count == 0);

		StableArray<Tracked> arr(16);
		arr.Resize(5);

		RR_CHECK(arr.Size() == 5);
		RR_CHECK(Tracked::live_count == 5);

		arr.Resize(2);
		RR_CHECK(arr.Size() == 2);
		RR_CHECK(Tracked::live_count == 2);

		arr.Resize(0);
		RR_CHECK(Tracked::live_count == 0);
	}

	void TestStableArray_ResizeWithValue()
	{
		StableArray<int> arr(16);
		arr.Resize(4, 7);

		RR_CHECK(arr.Size() == 4);
		for (size_t i = 0; i < arr.Size(); ++i)
		{
			RR_CHECK(arr[i] == 7);
		}
	}

	void TestStableArray_ReserveShrink()
	{
		StableArray<int> arr(2048);

		RR_CHECK(arr.Capacity() == 0);

		arr.Reserve(2000);
		RR_CHECK(arr.Capacity() >= 2000);

		size_t const cap_after_reserve = arr.Capacity();

		arr.Resize(2000, 1);
		RR_CHECK(arr.Size() == 2000);
		RR_CHECK(arr.Capacity() == cap_after_reserve);

		arr.Resize(10, 2);
		RR_CHECK(arr.Capacity() == cap_after_reserve);

		arr.ShrinkToFit();
		RR_CHECK(arr.Capacity() < cap_after_reserve);
		RR_CHECK(arr.Size() == 10);
	}

	void TestStableArray_MoveSemantics()
	{
		StableArray<int> a(16);
		a.PushBack(1);
		a.PushBack(2);
		a.PushBack(3);

		StableArray<int> b(std::move(a));

	#pragma warning(suppress : 26800)
		RR_CHECK(a.Size() == 0);
		RR_CHECK(b.Size() == 3);
		RR_CHECK(b[0] == 1 && b[1] == 2 && b[2] == 3);

		StableArray<int> c(16);
		c.PushBack(99);
		c = std::move(b);
		RR_CHECK(c.Size() == 3 && c[0] == 1);

		// self-move-assignment
		StableArray<int>& self_ref = c;
		c = std::move(self_ref);

	#pragma warning(suppress : 26800)
		RR_CHECK(c.Size() == 3 && c[0] == 1);
	}

	void TestStableArray_EmplaceMoveOnly()
	{
		StableArray<std::unique_ptr<int>> arr(8);

		arr.EmplaceBack(std::make_unique<int>(10));
		arr.EmplaceBack(std::make_unique<int>(20));
		arr.EmplaceBack(std::make_unique<int>(30));

		RR_CHECK(arr.Size() == 3);
		RR_CHECK(*arr[0] == 10 && *arr[1] == 20 && *arr[2] == 30);

		arr.PushBack(std::make_unique<int>(40));
		RR_CHECK(arr.Size() == 4 && *arr[3] == 40);
	}

	void TestStableArray_ConstCorrectness()
	{
		StableArray<int> arr(8);
		arr.PushBack(1);
		arr.PushBack(2);

		StableArray<int> const& const_ref = arr;

		RR_CHECK(const_ref[0] == 1);
		RR_CHECK(const_ref.Front() == 1);
		RR_CHECK(const_ref.Back() == 2);

		int const* const_data = const_ref.Data();
		RR_CHECK(const_data[0] == 1);

		//const_data[0] = 999; // compile-error
	}
}

namespace rr::test
{
	void TestStableArray()
	{
		LogOutput(LogLevel::Info, "=== StableArray Tests ===");
		TestStableArray_PushPopBasic();
		TestStableArray_DestructorCounts();
		TestStableArray_ResizeDefault();
		TestStableArray_ResizeWithValue();
		TestStableArray_ReserveShrink();
		TestStableArray_MoveSemantics();
		TestStableArray_EmplaceMoveOnly();
		TestStableArray_ConstCorrectness();
		LogOutput(LogLevel::Info, "=== StableArray Done ===");
	}
}