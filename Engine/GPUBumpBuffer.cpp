#include "pch.h"
#include "GPUBumpBuffer.h"
#include <MathUtils.h>
namespace rr
{
	GPUBumpBuffer::GPUBumpBuffer(GPUBumpHeap& gpu_heap, uint64_t byte_capacity) noexcept
	{
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(byte_capacity);
		resource_ = gpu_heap.CreatePlacedResource(desc);
		byte_capacity_ = byte_capacity;
	}

	uint64_t GPUBumpBuffer::Alloc(uint64_t byte_size) noexcept
	{
		uint64_t byte_offset_aligned = AlignUp(byte_offset_, 4Ui64);
		uint64_t byte_offset_next = byte_offset_aligned + byte_size;
		RR_CHECK(byte_offset_next <= byte_capacity_);

		byte_offset_ = byte_offset_next;
		return byte_offset_aligned;
	}
	uint64_t GPUBumpBuffer::Snapshot() const noexcept
	{
		return byte_offset_;
	}

	void GPUBumpBuffer::Restore(uint64_t snapshot) noexcept
	{
		RR_CHECK(snapshot <= byte_offset_);
		byte_offset_ = snapshot;
	}
}