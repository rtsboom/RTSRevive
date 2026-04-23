#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>

namespace rr
{
	struct StagingSubMesh
	{
		uint32_t m_index_count;
		uint32_t m_index_offset;
		uint32_t m_index_stride;

		uint32_t m_vertex_count;
		uint32_t m_position_offset;
		uint32_t m_normal_offset;
		uint32_t m_uv0_offset;

		int m_material_idx;
	};

	struct StagingMesh
	{
		uint16_t m_submesh_offset;
		uint16_t m_submesh_count;
	};

	struct MeshInstance
	{
		uint16_t m_node_idx;
		uint16_t m_mesh_idx;
	};

	struct StagingTexture
	{
		std::vector<std::byte> m_bytes;
		std::string            m_path;
	};

	struct StagingMaterial
	{
		DirectX::XMFLOAT3 m_emissive_factor = { 0, 0, 0 };
		DirectX::XMFLOAT4 m_base_color_factor = { 1, 1, 1, 1 };
		int      m_base_color_texture_idx = -1;

		float    m_metallic_factor = 0.0f;
		float    m_roughness_factor = 0.5f;
		int      m_metallic_roughness_texture_idx = -1;

		int      m_normal_texture_idx = -1;
		int      m_occlusion_texture_idx = -1;
	};

	struct StagingModel
	{
		std::vector<StagingMesh>     m_meshes;
		std::vector<StagingSubMesh>  m_submeshes;
		std::vector<MeshInstance>    m_instances;
		std::vector<StagingTexture>  m_textures;
		std::vector<StagingMaterial> m_materials;
		
		std::vector<DirectX::XMFLOAT3> m_positions;
		std::vector<DirectX::XMFLOAT3> m_normals;
		std::vector<DirectX::XMFLOAT2> m_uv0s;
		std::vector<uint32_t> m_indices_u32;
		std::vector<uint16_t> m_indices_u16;

		std::vector<DirectX::XMFLOAT4X4> m_transforms;
	};
}