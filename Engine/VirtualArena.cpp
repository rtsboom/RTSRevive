#include "pch.h"
#include "VirtualArena.h"
#include "VirtualMemory.h"
#include <cassert>
#include <cstddef>
#include <utility>

namespace
{
	size_t AlignUp(size_t value, size_t alignment)
	{
		assert((alignment & (alignment - 1)) == 0); // alignment must be a power of two
		return (value + alignment - 1) & ~(alignment - 1);
	}
}

namespace rr
{
	VirtualArena::VirtualArena(size_t max_size)
	{
		assert(max_size > 0);
		page_size_ = VirtualMemory::GetPageSize();

		size_t const allocation_granularity = VirtualMemory::GetAllocationGranularity();
		reserved_ = AlignUp(max_size, allocation_granularity);

		void* ptr = VirtualMemory::Reserve(reserved_);
		assert(ptr);

		data_ = static_cast<std::byte*>(ptr);
	}

	VirtualArena::~VirtualArena()
	{
		if (!data_)
			return;

		VirtualMemory::Release(data_);
	}
	VirtualArena::VirtualArena(VirtualArena&& other) noexcept
		: VirtualArena{}
	{
		Swap(other);
	}

	VirtualArena& VirtualArena::operator=(VirtualArena&& other) noexcept
	{
		VirtualArena intermidiate{ std::move(other) };
		Swap(intermidiate);
		return *this;
	}

	void VirtualArena::Clear() noexcept
	{
		used_ = 0;
	}
	void* VirtualArena::Allocate(size_t size, size_t alignment)
	{
		assert(data_);
		assert((alignment & (alignment - 1)) == 0); // alignment must be a power of two

		size_t const first = AlignUp(used_, alignment);
		size_t const last = first + size;

		if (last > reserved_)
			return nullptr;

		if (last > committed_)
		{
			size_t const page_aligned_last = AlignUp(last, page_size_);
			size_t const diff = page_aligned_last - committed_;
			VirtualMemory::Commit(data_ + committed_, diff);

			committed_ = page_aligned_last;
		}

		used_ = last;
		return data_ + first;
	}
	void VirtualArena::Swap(VirtualArena& other) noexcept
	{
		std::swap(data_, other.data_);
		std::swap(used_, other.used_);
		std::swap(committed_, other.committed_);
		std::swap(reserved_, other.reserved_);
		std::swap(page_size_, other.page_size_);
	}
}
