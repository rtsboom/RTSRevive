#include "pch.h"
#include "VirtualMemory.h"
#include <Windows.h>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <string>

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

	void* VirtualMemory::Reserve(size_t size)
	{
		assert(size > 0);

		void* p = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);

		if (!p)
		{
			DWORD err = GetLastError();
			throw std::runtime_error(
				"VirtualMemory::Reserve failed. GetLastError=" + std::to_string(err));
		}

		return p;
	}

	void VirtualMemory::Commit(void* ptr, size_t size)
	{
		assert(ptr);
		assert(size > 0);

		void* p = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
		if (!p)
		{
			DWORD err = GetLastError();
			throw std::runtime_error(
				"VirtualMemory::Commit failed. GetLastError=" + std::to_string(err));
		}
	}

	void VirtualMemory::Decommit(void* ptr, size_t size)
	{
		assert(ptr);
		assert(size > 0);

	#pragma warning(suppress: 6250) // intended to decommit memory, not release it
		BOOL success = VirtualFree(ptr, size, MEM_DECOMMIT);
		if (!success)
		{
			DWORD err = GetLastError();
			throw std::runtime_error(
				"VirtualMemory::Decommit failed. GeteLastError=" + std::to_string(err));
		}
	}
	void VirtualMemory::Release(void* ptr)
	{
		assert(ptr);

		BOOL success = VirtualFree(ptr, 0, MEM_RELEASE);
		if (!success)
		{
			DWORD err = GetLastError();
			throw std::runtime_error(
				"VirtualMemory::Release failed. GeteLastError=" + std::to_string(err));
		}
	}
}