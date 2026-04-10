#include "pch.h"
#include "ConstantBuffer.h"
#include "DxUtils.h"

static uint32_t AlignUp256(uint32_t value)
{
	return (value + 255) & ~255;
}

namespace rrv
{
	ConstantBuffer::~ConstantBuffer()
	{
		if (m_mapped) m_buffer->Unmap(0, nullptr);
	}

	ConstantBuffer::ConstantBuffer(ConstantBuffer&& other) noexcept
		: ConstantBuffer()
	{
		swap(*this, other);
	}

	ConstantBuffer& ConstantBuffer::operator=(ConstantBuffer&& other) noexcept
	{
		ConstantBuffer temp(std::move(other));
		swap(*this, temp);
		return *this;
	}

	void ConstantBuffer::Swap(ConstantBuffer& other) noexcept
	{
		using std::swap;
		swap(m_buffer, other.m_buffer);
		swap(m_bufferSize, other.m_bufferSize);
		swap(m_cursor, other.m_cursor);
		swap(m_gpuAddress, other.m_gpuAddress);
		swap(m_mapped, other.m_mapped);
	}

	void swap(ConstantBuffer& a, ConstantBuffer& b) noexcept
	{
		a.Swap(b);
	}

	ConstantBuffer::ConstantBuffer(ID3D12Device* device, uint32_t bufferSize)
	{
		m_bufferSize = bufferSize;
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

	ConstantBufferSlice ConstantBuffer::Allocate(uint32_t dataSize)
	{
		uint32_t alignedDataSize = AlignUp256(dataSize);

		if (m_cursor + alignedDataSize > m_bufferSize)
			return {};

		const uint64_t gpuAddress = m_gpuAddress + m_cursor;
		std::byte* const mapped = m_mapped + m_cursor;

		ConstantBufferSlice slice = { gpuAddress , mapped };
		m_cursor += alignedDataSize;

		return slice;
	}


}
