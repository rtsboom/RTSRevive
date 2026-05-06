#include "pch.h"
#include "GeometryRegistry.h"
#include <d3d12.h>
#include <cstdint>
#include <cstring>


namespace rr
{
	GeometryRegistry::GeometryRegistry(GPUBumpHeap& gpu_heap, uint32_t const* buffer_sizes)
	{
		std::memcpy(buffer_sizes_, buffer_sizes, sizeof(buffer_sizes_));
		// Create D3D12 Placed Buffers
		for (size_t i{}; i < sz(GeometryBuffers::Count); ++i)
		{
			D3D12_RESOURCE_DESC buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_sizes_[i]);

			//THROW_IF_FAILED(
			//	device->CreatePlacedResource(heap,
			//		heap_offset,
			//		&buffer_desc,
			//		D3D12_RESOURCE_STATE_COMMON,
			//		nullptr,
			//		IID_PPV_ARGS(&buffers_[i])));
		}


	}
}