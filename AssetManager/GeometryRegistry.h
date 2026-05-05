#pragma once
#include <Engine/GPUBumpHeap.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <CastUtils.h>


namespace rr
{
	using Microsoft::WRL::ComPtr;

	enum class GeometryBuffers
	{
		Index,
		Position,
		Surface,
		Skin,
		Count
	};

	// Global vertex and index streams 
	class GeometryRegistry
	{
	public:
		GeometryRegistry() = default;
		~GeometryRegistry() = default;
		GeometryRegistry(GeometryRegistry&&) = default;
		GeometryRegistry& operator=(GeometryRegistry&&) = default;
		GeometryRegistry(GeometryRegistry const&) = delete;
		GeometryRegistry& operator=(GeometryRegistry const&) = delete;

		GeometryRegistry(GPUBumpHeap& gpu_heap, uint32_t const* buffer_sizes);


		uint32_t Alloc(GeometryBuffers attr, uint32_t buffer_size) noexcept
		{
			uint32_t const offset = buffer_offsets_[sz(attr)];
			buffer_offsets_[sz(attr)] += buffer_size;
			return offset;
		}

		void Tell(uint32_t* buffer_offsets) const noexcept 
		{ 
			for (size_t i = 0; i < sz(GeometryBuffers::Count); ++i)
			{
				buffer_offsets[i] = buffer_offsets_[i];
			}
		}

		void Seek(uint32_t const* buffer_offsets) noexcept
		{
			for (size_t i = 0; i < sz(GeometryBuffers::Count); ++i)
			{
				buffer_offsets_[i] = buffer_offsets[i];
			}
		}

	private:
		ComPtr<ID3D12Resource> buffers_[sz(GeometryBuffers::Count)];
		uint32_t buffer_sizes_[sz(GeometryBuffers::Count)] = {};
		uint32_t buffer_offsets_[sz(GeometryBuffers::Count)] = {};
	};

}
