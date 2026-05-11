#pragma once
#include <Engine/GPUBumpBuffer.h>
#include <cstdint>
#include <CastUtils.h>


namespace rr
{
	using Microsoft::WRL::ComPtr;

	enum class GeometrySlot
	{
		Index,
		Position,
		Surface,
		Skin,
		Count
	};

	struct GeometryBuffers
	{
		GPUBumpBuffer index_buffer;
		GPUBumpBuffer position_buffer;
		GPUBumpBuffer surface_buffer;
		GPUBumpBuffer skin_buffer;
	};

	struct GeometrySnapshot
	{
		uint64_t buffer_snapshots[sz(GeometrySlot::Count)];
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

		GeometryRegistry(GeometryBuffers& pack);


		uint64_t Alloc(GeometrySlot slot, uint32_t byte_size) noexcept;
		uint64_t AllocIndexData(uint64_t byte_size) noexcept;
		uint64_t AllocPositionData(uint64_t byte_size) noexcept;
		uint64_t AllocSurfaceData(uint64_t byte_size) noexcept;
		uint64_t AllocSkinData(uint64_t byte_size) noexcept;

		GeometrySnapshot Snapshot() const noexcept;
		void Restore(GeometrySnapshot const& snapshot) noexcept;

	private:
		GPUBumpBuffer buffers_[sz(GeometrySlot::Count)] = {};
	};

}
