#pragma once
#include "AssetBase.h"
#include <cstdint>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <cstddef>
namespace rr
{
	using  VertexPosition = DirectX::XMFLOAT3;
	struct VertexSurface
	{
		DirectX::PackedVector::XMBYTEN4 normal;
		DirectX::PackedVector::XMBYTEN4 tangent;
		DirectX::XMFLOAT2 uv;
	};
	struct VertexSkin
	{
		DirectX::PackedVector::XMUBYTE4  joint;
		DirectX::PackedVector::XMUBYTEN4 weight;
	};


	struct Material
	{
		int base_color_texture_idx = -1;
		int metallic_roughness_texture_idx = -1;
		int normal_texture_idx = -1;
		int occlusion_texture_idx = -1;

		DirectX::XMFLOAT4 base_color_factor = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 emissive_factor = { 0.0f, 0.0f, 0.0f };
		float metallic_factor = 0.0f;
		float roughness_factor = 1.0f;
	};

	struct Primitive
	{
		uint32_t index_byte_offset;
		uint32_t index_byte_stride;
		uint32_t index_count;

		uint32_t position_base_idx;
		uint32_t surface_base_idx;
		uint32_t skin_base_idx;
		uint32_t vertex_count;

		uint32_t material_idx;
	};

	struct NodePrimitive
	{
		uint16_t node_idx;
		uint16_t primitive_idx;
	};

	struct AnimationClip
	{
		std::string name;
		float       duration;
		uint32_t    frame_count;
		uint32_t    node_count;
		uint32_t    node_matrix_base_idx;
	};

	struct ImageSource
	{
		std::unique_ptr<std::byte[]> data;
		uint32_t    len;
		bool        is_path;
	};

	struct ModelAsset : AssetBase
	{
		uint32_t node_count;
		std::vector<Material>      materials;
		std::vector<Primitive>     primitives;
		std::vector<NodePrimitive> node_primitives;
		std::vector<AnimationClip> animations;

		std::vector<VertexPosition>	     positions;
		std::vector<VertexSurface>	     surfaces;
		std::vector<VertexSkin>		     skins;
		std::vector<std::byte>		     indices;
		std::vector<DirectX::XMFLOAT4X4> node_matrices;

		std::vector<ImageSource> images;
	};
}