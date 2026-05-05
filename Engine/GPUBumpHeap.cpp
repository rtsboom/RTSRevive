#include "EnginePch.h"
#include "GPUBumpHeap.h"
#include <MathUtils.h>
#include <wrl/client.h>
#include <cstdint>
#include <stdexcept>
namespace rr
{
	GPUBumpHeap::GPUBumpHeap(ID3D12Device* device, D3D12_HEAP_DESC const& desc)
	{
		THROW_IF_FAILED(device->CreateHeap(&desc, IID_PPV_ARGS(&heap_)));
		device_ = device;
		heap_size_ = desc.SizeInBytes;
		heap_offset_ = 0;
	}

	ComPtr<ID3D12Resource> GPUBumpHeap::CreatePlacedResource(D3D12_RESOURCE_DESC desc)
	{
		ComPtr<ID3D12Resource> resource;
		D3D12_RESOURCE_ALLOCATION_INFO info = device_->GetResourceAllocationInfo(0, 1, &desc);
		if (UINT64_MAX == info.SizeInBytes)
		{
			throw std::runtime_error("GPUBumpHeap: Failed to get resource allocation info.");
		}

		uint64_t heap_offset_aligned = AlignUp(heap_offset_, info.Alignment);
		uint64_t heap_offset_next = heap_offset_aligned + info.SizeInBytes;

		if (heap_size_ < heap_offset_next)
		{
			throw std::runtime_error("GPUBumpHeap: Not enough space in heap to create placed resource.");
		}

		THROW_IF_FAILED(device_->CreatePlacedResource(
			heap_.Get(), heap_offset_aligned, &desc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));

		heap_offset_ = heap_offset_next;

		return resource;
	}
}