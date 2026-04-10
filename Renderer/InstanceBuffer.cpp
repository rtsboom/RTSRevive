#include "pch.h"
#include "InstanceBuffer.h"
#include "DxUtils.h"

namespace rrv
{
	InstanceBuffer::~InstanceBuffer()
	{
		if (m_mapped) m_buffer->Unmap(0, nullptr);
	}

	InstanceBuffer::InstanceBuffer(InstanceBuffer&& other) noexcept
		: InstanceBuffer()
	{
		swap(*this, other);
	}

	InstanceBuffer& InstanceBuffer::operator=(InstanceBuffer&& other) noexcept
	{
		InstanceBuffer temp(std::move(other));
		swap(*this, temp);
		return *this;
	}

	InstanceBuffer::InstanceBuffer(ID3D12Device* device, uint32_t stride, uint32_t count)
	{
		m_count = count;
		m_stride = stride;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(count * stride);

		THROW_IF_FAILED(device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&m_buffer)));

		m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped));
	}

	InstanceBufferSlice InstanceBuffer::Allocate(uint32_t count)
	{
		if (m_index + count > m_count)
			return {};

		std::byte* const mapped = m_mapped + m_index * m_stride;

		InstanceBufferSlice slice = { m_index, count, mapped };
		m_index += count;

		return slice;
	}

	void InstanceBuffer::Swap(InstanceBuffer& other) noexcept
	{
		using std::swap;
		swap(m_buffer, other.m_buffer);
		swap(m_stride, other.m_stride);
		swap(m_count, other.m_count);
		swap(m_index, other.m_index);
		swap(m_mapped, other.m_mapped);
	}

	void swap(InstanceBuffer& a, InstanceBuffer& b) noexcept
	{
		a.Swap(b);
	}
}