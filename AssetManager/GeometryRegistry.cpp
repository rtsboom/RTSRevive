#include "pch.h"
#include "GeometryRegistry.h"
#include <CastUtils.h>
#include <utility>
#include <cstdint>


namespace rr
{
	GeometryRegistry::GeometryRegistry(GeometryBuffers& pack)
	{
		buffers_[sz(GeometrySlot::Index)] = std::move(pack.index_buffer);
		buffers_[sz(GeometrySlot::Position)] = std::move(pack.position_buffer);
		buffers_[sz(GeometrySlot::Surface)] = std::move(pack.surface_buffer);
		buffers_[sz(GeometrySlot::Skin)] = std::move(pack.skin_buffer);
	}
	uint64_t GeometryRegistry::Alloc(GeometrySlot slot, uint32_t byte_size) noexcept
	{
		return buffers_[sz(slot)].Alloc(byte_size);
	}
	uint64_t GeometryRegistry::AllocIndexData(uint64_t byte_size) noexcept
	{
		return Alloc(GeometrySlot::Index, byte_size);
	}
	uint64_t GeometryRegistry::AllocPositionData(uint64_t byte_size) noexcept
	{
		return Alloc(GeometrySlot::Position, byte_size);
	}
	uint64_t GeometryRegistry::AllocSurfaceData(uint64_t byte_size) noexcept
	{
		return Alloc(GeometrySlot::Surface, byte_size);
	}
	uint64_t GeometryRegistry::AllocSkinData(uint64_t byte_size) noexcept
	{
		return Alloc(GeometrySlot::Skin, byte_size);
	}
	GeometrySnapshot GeometryRegistry::Snapshot() const noexcept
	{
		GeometrySnapshot snapshot = {};
		for (size_t i{}; i < sz(GeometrySlot::Count); ++i)
		{
			snapshot.buffer_snapshots[i] = buffers_[i].Snapshot();
		}

		return snapshot;
	}
	void GeometryRegistry::Restore(GeometrySnapshot const& snapshot) noexcept
	{
		for (size_t i{}; i < sz(GeometrySlot::Count); ++i)
		{
			buffers_[i].Restore(snapshot.buffer_snapshots[i]);
		}
	}
}