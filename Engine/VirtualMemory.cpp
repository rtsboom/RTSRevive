#include "pch.h"
#include "VirtualMemory.h"
#include "Asserts.h"
#include <Windows.h>

namespace
{
	SYSTEM_INFO const& GetSystemInfoCached()
	{
		static SYSTEM_INFO s_si = []
			{
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				return si;
			}();

		return s_si;
	}
}

namespace rr
{
	size_t VirtualMemory::GetPageSize()
	{
		return GetSystemInfoCached().dwPageSize;
	}

	size_t VirtualMemory::GetAllocationGranularity()
	{
		return GetSystemInfoCached().dwAllocationGranularity;
	}

	size_t VirtualMemory::AlignToPageSize(size_t size)
	{
		return AlignUp(size, GetPageSize());
	}

	size_t VirtualMemory::AlignToAllocationGranularity(size_t size)
	{
		return AlignUp(size, GetAllocationGranularity());
	}

	void* VirtualMemory::Reserve(size_t size)
	{
		RR_ASSERT(size > 0);
		RR_ASSERT(size == AlignToAllocationGranularity(size));

		void* p = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
		RR_CHECK_MSG_WIN32(
			p != nullptr, 
			"VirtualMemory::Reserve failed.");

		return p;
	}

	void VirtualMemory::Commit(void* ptr, size_t size)
	{
		RR_ASSERT(ptr != nullptr);
		RR_ASSERT(size > 0);
		RR_ASSERT(size == AlignToPageSize(size));

		RR_CHECK_MSG_WIN32(
			VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE), 
			"VirtualMemory::Commit failed.");
	}

	void VirtualMemory::Decommit(void* ptr, size_t size)
	{
		RR_ASSERT(ptr != nullptr);
		RR_ASSERT(size > 0);
		RR_ASSERT(size == AlignToPageSize(size));

	#pragma warning(suppress: 6250) // intended to decommit memory, not release it
		RR_CHECK_MSG_WIN32(
			VirtualFree(ptr, size, MEM_DECOMMIT),
			"VirtualMemory::Decommit failed.");
	}

	void VirtualMemory::Release(void* ptr)
	{
		RR_ASSERT(ptr != nullptr);

		RR_CHECK_MSG_WIN32(
			VirtualFree(ptr, 0, MEM_RELEASE),
			"VirtualMemory::Release failed.");
	}
}