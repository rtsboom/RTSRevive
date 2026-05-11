#pragma once
#include <d3d12.h>
#include <cstdint>
#include <wrl/client.h>
namespace rr
{
	using Microsoft::WRL::ComPtr;
	class GPUBumpHeap
	{
	public:
		GPUBumpHeap() = default;
		~GPUBumpHeap() = default;
		GPUBumpHeap(GPUBumpHeap&&) = default;
		GPUBumpHeap& operator=(GPUBumpHeap&&) = default;
		GPUBumpHeap(GPUBumpHeap const&) = delete;
		GPUBumpHeap& operator=(GPUBumpHeap const&) = delete;
		
		GPUBumpHeap(ID3D12Device* device, D3D12_HEAP_DESC const& desc);
		ComPtr<ID3D12Resource> CreatePlacedResource(D3D12_RESOURCE_DESC desc);

	private:
		ID3D12Device* device_{};
		ComPtr<ID3D12Heap> heap_{};
		uint64_t		   byte_capacity_{};
		uint64_t		   byte_offset_{};
	};

}
