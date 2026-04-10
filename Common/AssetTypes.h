#pragma once
#include "CastUtils.h"
#include <cstdint>
#include <vector>

namespace rrv::Asset
{
	struct Geometry
	{
		enum class BufferSlot { POSITION, NORMAL, UV, INDEX, COUNT };
		static inline constexpr const char* attribute_names[]
		{
			"POSITION", "NORMAL", "TEXCOORD_0"
		};
		struct BufferView { uint32_t offset, stride; };

		uint32_t index_count;
		uint32_t vertex_count;
		BufferView views[u64(BufferSlot::COUNT)];
	};

	struct Material
	{
		int   base_color_texture_idx;
		float metallic_factor;
		float roughness_factor;
		bool  has_metallic_factor;
		bool  has_roughness_factor;
	};

	struct Mesh
	{
		uint32_t start;
		uint32_t count;
	};

	struct RenderItem
	{
		uint32_t mesh_idx;
		uint32_t node_idx;
	};

	struct Model
	{
		std::vector<Geometry>		geometries;
		std::vector<Material>		materials;
		std::vector<Mesh>			meshes;

		std::vector<RenderItem>		render_items;
	};
}