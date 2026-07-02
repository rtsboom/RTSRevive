#include "pch.h"
#include "DynamicArena.h"
#include "VirtualMemory.h"
#include <cassert>
#include <cstddef>
#include <utility>

namespace rr
{
	DynamicArena::DynamicArena(size_t max_size)
	{
		assert(max_size > 0);

		size_t const max_size_aligned = VirtualMemory::AlignToAllocationGranularity(max_size);

		void* ptr = VirtualMemory::Reserve(max_size_aligned);
		assert(ptr);

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
		if (!data_)
			return;

		VirtualMemory::Release(data_);
	}

	void DynamicArena::Clear() noexcept
	{
		used_bytes = 0;
	}

	void DynamicArena::Reset()
	{
		assert(data_);

		VirtualMemory::Decommit(data_, committed_bytes_);
		committed_bytes_ = 0;
		used_bytes = 0;
	}
	void* DynamicArena::Allocate(size_t size, size_t alignment)
	{
		assert(data_);
		assert(size > 0);

		// alignment must be a power of two
		assert(alignment > 0 && (alignment & (alignment - 1)) == 0); 

		size_t const start_bytes = (used_bytes + alignment - 1) & ~(alignment - 1);
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
