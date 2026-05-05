#include "EnginePch.h"
#include "GPUBuffer.h"
#include <d3d12.h>
#include <cstdint>

namespace rr
{
	GPUBuffer::GPUBuffer(ID3D12Device* device, uint64_t byte_size) noexcept
	{
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byte_size);
		THROW_IF_FAILED(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&resource_)));
	}
}
