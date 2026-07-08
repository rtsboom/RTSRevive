#include "pch.h"
#include "StableArena.h"
#include "VirtualMemory.h"
#include "Asserts.h"
#include <cstddef>
#include <utility>

namespace rr
{
	StableArena::StableArena(size_t max_size)
	{
		RR_ASSERT(max_size > 0);

		size_t const max_size_aligned = VirtualMemory::AlignToAllocationGranularity(max_size);

		void* ptr = VirtualMemory::Reserve(max_size_aligned);

		data_ = static_cast<std::byte*>(ptr);
		reserved_bytes_ = max_size_aligned;
	}
	
	StableArena::~StableArena()
	{
		if (data_)
		{
			VirtualMemory::Release(data_);
		}
	}

	void StableArena::Rollback(size_t marker)
	{
		RR_CHECK(marker <= used_bytes_);

		used_bytes_ = marker;
	}

	void StableArena::Decommit()
	{
		RR_ASSERT(data_ != nullptr);

		VirtualMemory::Decommit(data_, committed_bytes_);
		committed_bytes_ = 0;
		used_bytes_ = 0;
	}
	void* StableArena::Allocate(size_t size, size_t alignment)
	{
		RR_ASSERT(data_ != nullptr);
		RR_ASSERT(size > 0);
		RR_ASSERT(IsPowerOfTwo(alignment));

		size_t const start_bytes = AlignUp(used_bytes_, alignment);
		size_t const finish_bytes = start_bytes + size;

		if (reserved_bytes_ < finish_bytes)
			return nullptr;

		if (committed_bytes_ < finish_bytes)
		{
			size_t const finish_bytes_aligned = VirtualMemory::AlignToPageSize(finish_bytes);

			std::byte* const committed_end = data_ + committed_bytes_;
			size_t const commit_bytes = finish_bytes_aligned - committed_bytes_;
			VirtualMemory::Commit(committed_end, commit_bytes);

			committed_bytes_ = finish_bytes_aligned;
		}

		used_bytes_ = finish_bytes;

		std::byte* const allocation_begin = data_ + start_bytes;
		return allocation_begin;
	}
}
