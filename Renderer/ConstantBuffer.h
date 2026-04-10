#pragma once
#include <d3d12.h>
#include <cstdint>
#include <cstddef>

namespace rrv
{
	struct ConstantBufferSlice
	{
		uint64_t gpuAddress = 0;
		std::byte* mapped = nullptr;
	};

	class ConstantBuffer
	{
	public:
		~ConstantBuffer();
		ConstantBuffer() = default;
		ConstantBuffer(const ConstantBuffer&) = delete;
		ConstantBuffer& operator=(const ConstantBuffer&) = delete;
		ConstantBuffer(ConstantBuffer&&) noexcept;
		ConstantBuffer& operator=(ConstantBuffer&&) noexcept;
	public:
		void Swap(ConstantBuffer& other) noexcept;
		friend void swap(ConstantBuffer& a, ConstantBuffer& b) noexcept;
	public:
		ConstantBuffer(ID3D12Device* device, uint32_t bufferSize);
		ConstantBufferSlice Allocate(uint32_t dataSize);


	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
		uint32_t m_bufferSize = 0;
		uint32_t     m_cursor = 0;
		uint64_t m_gpuAddress = 0;
		std::byte* m_mapped = nullptr;
	};

}
