#pragma once
namespace rr
{
	class VirtualMemory
	{
	public:
		static size_t GetPageSize();

		static void* Reserve(size_t size);
		static void  Commit(void* ptr, size_t size);
		static void  Decommit(void* ptr, size_t size);
		static void  Release(void* ptr);
	};
}

