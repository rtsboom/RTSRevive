#pragma once
#include <d3d12.h>
#include <cstdint>
#include <cstddef>

namespace rrv
{
	struct InstanceBufferSlice
	{
		uint32_t index = 0;
		uint32_t count = 0;
		std::byte* mapped = nullptr;
	};


	class InstanceBuffer
	{
	public:
		InstanceBuffer() = default;
		~InstanceBuffer();
		InstanceBuffer(InstanceBuffer&&) noexcept;
		InstanceBuffer& operator=(InstanceBuffer&&) noexcept;

		// non-copyable
		InstanceBuffer(const InstanceBuffer&) = delete;
		InstanceBuffer& operator=(const InstanceBuffer&) = delete;


		InstanceBuffer(ID3D12Device* device, uint32_t stride, uint32_t count);
		InstanceBufferSlice Allocate(uint32_t count);

		ID3D12Resource* GetResource() const noexcept { return m_buffer.Get(); }


	public:
		void Swap(InstanceBuffer& other) noexcept;
		friend void swap(InstanceBuffer& a, InstanceBuffer& b) noexcept;

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
		uint32_t m_stride = 0;
		uint32_t m_count = 0;
		uint32_t m_index = 0;
		std::byte* m_mapped = nullptr;

	};
}

