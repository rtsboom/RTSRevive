#pragma once
#include "GPUBumpHeap.h"
#include <CastUtils.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

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




	private:

	};
}
