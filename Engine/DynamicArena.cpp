#include "pch.h"
#include "DynamicArena.h"
#include "VirtualMemory.h"
#include "Asserts.h"
#include <cstddef>
#include <utility>

namespace rr
{
	DynamicArena::DynamicArena(size_t max_size)
	{
		RR_ASSERT(max_size > 0);

		size_t const max_size_aligned = VirtualMemory::AlignToAllocationGranularity(max_size);

		void* ptr = VirtualMemory::Reserve(max_size_aligned);

		data_ = static_cast<std::byte*>(ptr);
		reserved_bytes_ = max_size_aligned;
	}
	DynamicArena::DynamicArena(DynamicArena&& other) noexcept
		: DynamicArena{}
	{
		Swap(other);
	}
	DynamicArena& DynamicArena::operator=(DynamicArena&& other) noexcept
	{
		DynamicArena intermidiate{ std::move(other) };
		Swap(intermidiate);
		return *this;
	}

	DynamicArena::~DynamicArena()
	{
		if (data_)
		{
			VirtualMemory::Release(data_);
		}
	}

	void DynamicArena::Rollback(size_t marker)
	{
		RR_CHECK(marker <= used_bytes);

		used_bytes = marker;
	}

	void DynamicArena::Reset()
	{
		RR_ASSERT(data_ != nullptr);

		VirtualMemory::Decommit(data_, committed_bytes_);
		committed_bytes_ = 0;
		used_bytes = 0;
	}
	void* DynamicArena::Allocate(size_t size, size_t alignment)
	{
		RR_ASSERT(data_ != nullptr);
		RR_ASSERT(size > 0);
		RR_ASSERT(IsPowerOfTwo(alignment));

		size_t const start_bytes = AlignUp(used_bytes, alignment);
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

		used_bytes = finish_bytes;

		std::byte* const allocation_begin = data_ + start_bytes;
		return allocation_begin;
	}

	void DynamicArena::Swap(DynamicArena& other) noexcept
	{
		std::swap(data_, other.data_);
		std::swap(used_bytes, other.used_bytes);
		std::swap(committed_bytes_, other.committed_bytes_);
		std::swap(reserved_bytes_, other.reserved_bytes_);
	}
}
