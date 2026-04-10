#pragma once
#include <cstddef>

namespace rrv
{
	using Microsoft::WRL::ComPtr;
	class GpuUploadBuffer
	{
	public:
		~GpuUploadBuffer();
		GpuUploadBuffer() = default;
		GpuUploadBuffer(GpuUploadBuffer&&) noexcept;
		GpuUploadBuffer& operator=(GpuUploadBuffer&&) noexcept;
		GpuUploadBuffer(const GpuUploadBuffer&) = delete;
		GpuUploadBuffer& operator=(const GpuUploadBuffer&) = delete;
	public:
		GpuUploadBuffer(ID3D12Device* device, uint64_t sizeInBytes);
		ID3D12Resource* Get() const noexcept { return m_buffer.Get(); }
		uint64_t GetSize() const noexcept { return m_bufferSize; }
		uint64_t GetGpuAddress() const noexcept { return m_gpuAddress; }
		std::byte* GetMapped() const noexcept { return m_mapped; }
	public:
		void Swap(GpuUploadBuffer& other) noexcept;
		friend void swap(GpuUploadBuffer& a, GpuUploadBuffer& b) noexcept;

	private:
		ComPtr<ID3D12Resource> m_buffer;
		uint64_t   m_bufferSize = 0;
		uint64_t   m_gpuAddress = 0;
		std::byte* m_mapped = nullptr;
	};
}
