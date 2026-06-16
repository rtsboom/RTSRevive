#pragma once
#include "GPUBumpHeap.h"

#pragma warning(push, 0)
#include <d3d12.h>
#pragma warning(pop)

#include <wrl/client.h>
#include <type_traits>
#include <cstdint>
namespace rr
{

	using Microsoft::WRL::ComPtr;

	class GPUBumpBuffer
	{ 
	public:
		// move only
		GPUBumpBuffer() = default;
		~GPUBumpBuffer() = default;
		GPUBumpBuffer(GPUBumpBuffer&&) = default;
		GPUBumpBuffer& operator=(GPUBumpBuffer&&) = default;

		GPUBumpBuffer(GPUBumpHeap& gpu_heap, uint64_t byte_capacity) noexcept;
		uint64_t Alloc(uint64_t byte_size) noexcept;
		uint64_t Snapshot() const noexcept;
		void Restore(uint64_t snapshot) noexcept;

		ID3D12Resource* Resource() const noexcept { return resource_.Get(); }
	private:
		ComPtr<ID3D12Resource> resource_{};
		uint64_t byte_capacity_{};
		uint64_t byte_offset_{};
	};

	static_assert(std::is_nothrow_move_constructible_v<GPUBumpBuffer>);
	static_assert(std::is_nothrow_move_assignable_v<GPUBumpBuffer>);
}

