#pragma once
#include <vector>
#include <cstdint>
#include <CastUtils.h>
#include <DirectXMath.h>

namespace rr
{
	struct ByteSpan
	{
		uint32_t offset;
		uint32_t length;
		uint32_t stride;
	};

	namespace Asset
	{
		struct Mesh
		{
			enum class View { POSITION, NORMAL, UV, INDEX, COUNT };

			rr::ByteSpan views[u64(View::COUNT)];
			int          material_idx;
			uint32_t     vertex_count;
			uint32_t     index_count;
		};

		struct Material
		{
			int   base_color_texture_idx;
			float metallic_factor;
			float roughness_factor;
			bool  has_metallic_factor;
			bool  has_roughness_factor;
		};

		struct MeshNode { uint16_t mesh_idx, node_idx; };

		struct Model
		{
			std::vector<rr::Asset::Material> materials;
			std::vector<rr::Asset::Mesh>     meshes;

			std::vector<DirectX::XMFLOAT4X4> world_matrices; // flatten node hierarchy
			std::vector<MeshNode>            mesh_nodes;
		};
	}
}