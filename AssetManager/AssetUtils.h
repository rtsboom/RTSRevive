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

	enum class VertexAttribute // Can be extended with more attributes as needed.
	{
		POSITION,
		NORMAL,
		UV,
		COUNT
	};

	struct MeshAsset
	{
		ByteSpan views[u64(VertexAttribute::COUNT)];
		int      position;
		int      normal;
		int      uv0;
		int      indices;
		int      material;
		uint32_t vertex_count;
		uint32_t index_count;
	};

	struct MaterialAsset
	{
		int   base_color_texture_idx;
		float metallic_factor;
		float roughness_factor;
		bool  has_metallic_factor;
		bool  has_roughness_factor;
	};

	struct MeshNode { uint16_t mesh_idx, node_idx; };

	struct ModelAsset
	{
		std::vector<ByteSpan>      buffer_views;
		std::vector<MaterialAsset> materials;
		std::vector<MeshAsset>     meshes;

		std::vector<DirectX::XMFLOAT4X4> node_world_transforms; // flatten node hierarchy
		std::vector<MeshNode>            mesh_nodes;
	};

}