#pragma once
namespace rr
{
	class VirtualMemory
	{
	public:
		static size_t GetPageSize();
		static size_t GetAllocationGranularity();
		static size_t AlignToPageSize(size_t size);
		static size_t AlignToAllocationGranularity(size_t size);

		static void* Reserve(size_t size);
		static void  Commit(void* ptr, size_t size);
		static void  Decommit(void* ptr, size_t size);
		static void  Release(void* ptr);
	};
}

