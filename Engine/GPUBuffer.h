#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
namespace rr
{
	using Microsoft::WRL::ComPtr;
	class GPUBuffer
	{
	public:
		GPUBuffer() noexcept = default;
		~GPUBuffer() noexcept = default;
		GPUBuffer(GPUBuffer&&) noexcept = default;
		GPUBuffer& operator=(GPUBuffer&&) noexcept = default;

		// Non Copyable
		GPUBuffer(const GPUBuffer&) = delete;
		GPUBuffer& operator=(const GPUBuffer&) = delete;

		GPUBuffer(ID3D12Device* device, uint64_t byte_size) noexcept;

		ID3D12Resource* GetResource() const noexcept { return resource_.Get(); }

	private:
		ComPtr<ID3D12Resource> resource_;
	};
}

