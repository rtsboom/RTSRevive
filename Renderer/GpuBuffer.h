#pragma once
namespace rrv
{
	using Microsoft::WRL::ComPtr;
	class GpuBuffer
	{
	public:
		GpuBuffer() = default;
		GpuBuffer(GpuBuffer&&) = default;
		GpuBuffer& operator=(GpuBuffer&&) = default;
		GpuBuffer(const GpuBuffer&) = delete;
		GpuBuffer& operator=(const GpuBuffer&) = delete;
	public:
		GpuBuffer(ID3D12Device* device, uint64_t bufferSize);
		ID3D12Resource* Get() const noexcept { return m_buffer.Get(); }
		uint64_t GetSize() const noexcept { return m_bufferSize; }
		uint64_t GetGpuAddress() const noexcept { return m_gpuAddress; }

	protected:
		ComPtr<ID3D12Resource> m_buffer;
		uint64_t m_gpuAddress = 0;
		uint64_t m_bufferSize = 0;
	};

}
