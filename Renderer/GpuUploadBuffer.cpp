#include "pch.h"
#include "GpuUploadBuffer.h"
#include "DxUtils.h"

namespace rrv
{
	GpuUploadBuffer::~GpuUploadBuffer()
	{
		if (m_mapped) m_buffer->Unmap(0, nullptr);
	}

	GpuUploadBuffer::GpuUploadBuffer(GpuUploadBuffer&& other) noexcept
		: GpuUploadBuffer()
	{
		swap(*this, other);
	}

	GpuUploadBuffer& GpuUploadBuffer::operator=(GpuUploadBuffer&& other) noexcept
	{
		GpuUploadBuffer temp(std::move(other));
		swap(*this, temp);
		return *this;
	}

	GpuUploadBuffer::GpuUploadBuffer(ID3D12Device* device, uint64_t bufferSize)
		: m_bufferSize(bufferSize)
	{
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		THROW_IF_FAILED(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_buffer)));

		THROW_IF_FAILED(m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped)));
		m_gpuAddress = m_buffer->GetGPUVirtualAddress();
	}

	void GpuUploadBuffer::Swap(GpuUploadBuffer& other) noexcept
	{
		using std::swap;
		swap(m_buffer, other.m_buffer);
		swap(m_bufferSize, other.m_bufferSize);
		swap(m_gpuAddress, other.m_gpuAddress);
		swap(m_mapped, other.m_mapped);
	}

	void swap(GpuUploadBuffer& a, GpuUploadBuffer& b) noexcept
	{
		a.Swap(b);
	}

}

