#pragma once
#include "GPUBumpBuffer.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace rr
{
	enum class GPUGlobalBuffers
	{
		Index,
		Position,
		Surface,
		Skinning,
		Material,
		Primitive,
		Count
	};

	using Microsoft::WRL::ComPtr;
	class GPURegistry
	{
	public:
		GPURegistry() = default;
		~GPURegistry() = default;
		GPURegistry(GPURegistry&&) = default;
		GPURegistry& operator=(GPURegistry&&) = default;

		// Non Copyable
		GPURegistry(GPURegistry const&) = delete;
		GPURegistry& operator=(GPURegistry const&) = delete;

		// Add gpu buffer; Return gpu buffer index;
		uint64_t AddBuffer(GPUBumpBuffer&& gpu_buffer) noexcept;
		GPUBumpBuffer& GetBuffer(uint64_t handle) noexcept;
	private:
		std::vector<GPUBumpBuffer> buffers_{};

		ComPtr<ID3D12DescriptorHeap> descriptors_;
	};
}
