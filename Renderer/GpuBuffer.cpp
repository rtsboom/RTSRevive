#include "pch.h"
#include "GpuBuffer.h"
#include "DxUtils.h"

namespace rrv
{
	GpuBuffer::GpuBuffer(ID3D12Device* device, uint64_t bufferSize)
	{
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		THROW_IF_FAILED(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_buffer)));
		m_gpuAddress = m_buffer->GetGPUVirtualAddress();
		m_bufferSize = bufferSize;
	}
}

