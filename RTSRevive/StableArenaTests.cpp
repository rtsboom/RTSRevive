#include "pch.h"
#include <Engine/StableArena.h>
#include <Engine/Asserts.h>

namespace
{
	using namespace rr;

	struct TestObject
	{
		int a;
		float b;
		TestObject() = default;
		TestObject(int a, float b)
			: a{ a }, b{ b }
		{
		}
	};

	static bool IsAligned(void* ptr, size_t alignment)
	{
		return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
	}

	void TestStableArena_BasicAllocate()
	{
		rr::StableArena arena{ 1024 * 1024 };

		RR_CHECK(arena.ReservedSize() >= 1024 * 1024);
		RR_CHECK(arena.UsedSize() == 0);
		RR_CHECK(arena.CommittedSize() == 0);

		void* p = arena.Allocate(64, 16);

		RR_CHECK(p != nullptr);
		RR_CHECK(IsAligned(p, 16));
		RR_CHECK(arena.UsedSize() >= 64);
		RR_CHECK(arena.CommittedSize() >= arena.UsedSize());
	}

	void TestStableArena_Alignment()
	{
		rr::StableArena arena{ 1024 * 1024 };

		void* p1 = arena.Allocate(1, 1);
		void* p2 = arena.Allocate(1, 8);
		void* p3 = arena.Allocate(1, 16);
		void* p4 = arena.Allocate(1, 64);

		RR_CHECK(p1 != nullptr);
		RR_CHECK(p2 != nullptr);
		RR_CHECK(p3 != nullptr);
		RR_CHECK(p4 != nullptr);

		RR_CHECK(IsAligned(p1, 1));
		RR_CHECK(IsAligned(p2, 8));
		RR_CHECK(IsAligned(p3, 16));
		RR_CHECK(IsAligned(p4, 64));
	}

	void TestStableArena_New()
	{
		rr::StableArena arena{ 1024 * 1024 };

		TestObject* obj = arena.New<TestObject>(10, 3.5f);

		RR_CHECK(obj != nullptr);
		RR_CHECK(obj->a == 10);
		RR_CHECK(obj->b == 3.5f);
	}

	void TestStableArena_NewArray()
	{
		rr::StableArena arena{ 1024 * 1024 };

		TestObject* arr = arena.NewArray<TestObject>(8);

		RR_CHECK(arr != nullptr);

		for (int i = 0; i < 8; ++i)
		{
			RR_CHECK(arr[i].a == 0);
			RR_CHECK(arr[i].b == 0);
		}
	}

	void TestStableArena_Rollback()
	{
		rr::StableArena arena{ 1024 * 1024 };

		void* p1 = arena.Allocate(64, 16);
		size_t marker = arena.UsedSize();

		void* p2 = arena.Allocate(128, 16);
		RR_CHECK(p1 != nullptr);
		RR_CHECK(p2 != nullptr);
		RR_CHECK(arena.UsedSize() > marker);

		arena.Rollback(marker);

		RR_CHECK(arena.UsedSize() == marker);

		void* p3 = arena.Allocate(128, 16);

		RR_CHECK(p3 == p2);
	}

	void TestStableArena_Rewind()
	{
		rr::StableArena arena{ 1024 * 1024 };

		void* p1 = arena.Allocate(64, 16);
		RR_CHECK(p1 != nullptr);
		RR_CHECK(arena.UsedSize() > 0);

		arena.Rewind();

		RR_CHECK(arena.UsedSize() == 0);

		void* p2 = arena.Allocate(64, 16);

		RR_CHECK(p2 == p1);
	}

	void TestStableArena_Decommit()
	{
		rr::StableArena arena{ 1024 * 1024 };

		void* p = arena.Allocate(64 * 1024, 16);

		RR_CHECK(p != nullptr);
		RR_CHECK(arena.UsedSize() > 0);
		RR_CHECK(arena.CommittedSize() >= arena.UsedSize());

		arena.Rewind();
		arena.Decommit();

		RR_CHECK(arena.UsedSize() == 0);
		RR_CHECK(arena.CommittedSize() == 0);
		RR_CHECK(arena.ReservedSize() >= 1024 * 1024);
	}

	void TestStableArena_MoveConstructor()
	{
		rr::StableArena arena{ 1024 * 1024 };

		void* p1 = arena.Allocate(64, 16);
		size_t used = arena.UsedSize();
		size_t committed = arena.CommittedSize();
		size_t reserved = arena.ReservedSize();

		rr::StableArena moved{ std::move(arena) };

		RR_CHECK(moved.UsedSize() == used);
		RR_CHECK(moved.CommittedSize() == committed);
		RR_CHECK(moved.ReservedSize() == reserved);

		void* p2 = moved.Allocate(64, 16);

		RR_CHECK(p1 != nullptr);
		RR_CHECK(p2 != nullptr);
	}

	void TestStableArena_MoveAssignment()
	{
		rr::StableArena a{ 1024 * 1024 };
		rr::StableArena b{ 1024 * 1024 };

		void* old_b = b.Allocate(32, 16);
		void* old_a = a.Allocate(64, 16);

		size_t used = a.UsedSize();
		size_t committed = a.CommittedSize();
		size_t reserved = a.ReservedSize();

		b = std::move(a);

		RR_CHECK(b.UsedSize() == used);
		RR_CHECK(b.CommittedSize() == committed);
		RR_CHECK(b.ReservedSize() == reserved);

		void* p = b.Allocate(64, 16);

		RR_CHECK(old_a != nullptr);
		RR_CHECK(old_b != nullptr);
		RR_CHECK(p != nullptr);
	}
}

namespace rr::test
{
	void TestStableArena()
	{
		LogOutput(LogLevel::Info, "=== StableArena Tests ===");
		TestStableArena_BasicAllocate();
		TestStableArena_Alignment();
		TestStableArena_New();
		TestStableArena_NewArray();
		TestStableArena_Rollback();
		TestStableArena_Rewind();
		TestStableArena_Decommit();
		TestStableArena_MoveConstructor();
		TestStableArena_MoveAssignment();
		LogOutput(LogLevel::Info, "=== StableArena Done ===");
	}
}