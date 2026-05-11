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
		RR_D3D_CHECK(device->CreateHeap(&desc, IID_PPV_ARGS(&heap_)));
		device_ = device;
		byte_capacity_ = desc.SizeInBytes;
	}

	ComPtr<ID3D12Resource> GPUBumpHeap::CreatePlacedResource(D3D12_RESOURCE_DESC desc)
	{
		ComPtr<ID3D12Resource> resource;
		D3D12_RESOURCE_ALLOCATION_INFO info = device_->GetResourceAllocationInfo(0, 1, &desc);
		RR_CHECK(info.SizeInBytes != UINT64_MAX);

		uint64_t byte_offset_aligned = AlignUp(byte_offset_, info.Alignment);
		uint64_t byte_offset_next = byte_offset_aligned + info.SizeInBytes;
		RR_CHECK(byte_offset_next <= byte_capacity_);

		RR_D3D_CHECK(device_->CreatePlacedResource(
			heap_.Get(), byte_offset_aligned, &desc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));

		byte_offset_ = byte_offset_next;
		return resource;
	}
}