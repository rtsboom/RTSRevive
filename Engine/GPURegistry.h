#pragma once
#include <CastUtils.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>

namespace rr
{
	using Microsoft::WRL::ComPtr;
	class GPURegistry
	{
	private:
		enum class GlobalBuffers
		{
			Index,
			Position,
			Surface,
			Skinning,
			Material,
			Primitive,
			Count
		};
	public:
		GPURegistry() = default;
		~GPURegistry() = default;
		GPURegistry(GPURegistry&&) = default;
		GPURegistry& operator=(GPURegistry&&) = default;

		// Non Copyable
		GPURegistry(GPURegistry const&) = delete;
		GPURegistry& operator=(GPURegistry const&) = delete;

	public:
		size_t AllocIndex(size_t byte_length);
		size_t AllocPosition(size_t byte_length);
		size_t AllocSurface(size_t byte_length);
		size_t AllocSkinning(size_t byte_length);
		size_t AllocMaterial(size_t byte_length);
		size_t AllocPrimitive(size_t byte_length);


	private:
		std::array<ComPtr<ID3D12Resource>, 
			sz(GlobalBuffers::Count)> buffers_;


	};
}
