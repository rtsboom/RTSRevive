#include "pch.h"
#include "GLTFLoader.h"
#include "StagingModel.h"
#include <CastUtils.h>

#pragma warning(push, 0)
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>
#pragma warning(pop)

#include <DirectXMath.h>

#include <Windows.h>
#include <utility>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
	using namespace DirectX;
	using namespace rr;

	XMMATRIX GetLocalMatrix(tinygltf::Node const& node)
	{
		if (node.matrix.size() == 16)
		{
			// gltf matrix is column-major;
			// copying sequentially into row-major XMFLOAT4X4 implicitly transposes it;
			XMMATRIX m = XMMATRIX(
				f32(node.matrix[0]), f32(node.matrix[1]), f32(node.matrix[2]), f32(node.matrix[3]),
				f32(node.matrix[4]), f32(node.matrix[5]), f32(node.matrix[6]), f32(node.matrix[7]),
				f32(node.matrix[8]), f32(node.matrix[9]), f32(node.matrix[10]), f32(node.matrix[11]),
				f32(node.matrix[12]), f32(node.matrix[13]), f32(node.matrix[14]), f32(node.matrix[15])
			);
			return m;
		}

		// S * R * T
		XMMATRIX m = XMMatrixIdentity();
		if (node.scale.size() == 3)
		{
			m *= XMMatrixScaling(
				f32(node.scale[0]),
				f32(node.scale[1]),
				f32(node.scale[2]));
		}
		if (node.rotation.size() == 4)
		{
			XMVECTOR q = XMVectorSet(
				f32(node.rotation[0]),
				f32(node.rotation[1]),
				f32(node.rotation[2]),
				f32(node.rotation[3]));
			m *= XMMatrixRotationQuaternion(q);
		}
		if (node.translation.size() == 3)
		{
			m *= XMMatrixTranslation(
				f32(node.translation[0]),
				f32(node.translation[1]),
				f32(node.translation[2]));
		}

		return m;
	}

	std::vector<XMFLOAT4X4> ComputeWorldTransforms(tinygltf::Model const& gltf)
	{
		const uint32_t node_count = u32(gltf.nodes.size());
		std::vector<XMFLOAT4X4> out_matrices(node_count);

		std::vector<int> parents(node_count, -1);

		for (uint32_t i = 0; i < node_count; ++i)
		{
			for (int child : gltf.nodes[i].children)
			{
				parents[child] = i;
			}
		}

		std::vector<int> visit_stack;
		visit_stack.reserve(node_count);

		for (int root : gltf.scenes.front().nodes)
		{
			visit_stack.push_back(root);
		}

		while (!visit_stack.empty())
		{
			const int top = visit_stack.back();
			visit_stack.pop_back();

			auto& current_node = gltf.nodes[top];
			XMMATRIX m = GetLocalMatrix(current_node);
			if (int parent = parents[top]; parent != -1)
			{
				m *= XMLoadFloat4x4(&out_matrices[parent]);
			}
			XMStoreFloat4x4(&out_matrices[top], m);

			for (int child : current_node.children)
			{
				visit_stack.push_back(child);
			}
		}

		return out_matrices;
	}

	void ConvertRhToLhVector3(std::vector<XMFLOAT3>& vecs)
	{
		for (auto& v : vecs)
		{
			v.z = -v.z;
		}
	}

	void ConvertRhToLhTransforms(std::vector<XMFLOAT4X4>& transforms)
	{
		// S = Scale(1, 1, -1)
		// S * M * S
		for (auto& transform : transforms)
		{
			transform._13 = -transform._13;
			transform._23 = -transform._23;
			transform._43 = -transform._43;

			transform._31 = -transform._31;
			transform._32 = -transform._32;
			transform._34 = -transform._34;
		}
	}

	void ConvertWindingOrder(std::vector<uint32_t>& indices)
	{
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			std::swap(indices[i + 1], indices[i + 2]);
		}
	}

	void ConvertWindingOrder(std::vector<uint16_t>& indices)
	{
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			std::swap(indices[i + 1], indices[i + 2]);
		}
	}

	void ProcessMeshes(StagingModel& staging, tinygltf::Model const& gltf)
	{
		staging.m_meshes.resize(gltf.meshes.size());
		size_t all_submesh_count = 0;
		for (int mesh_idx = 0; mesh_idx < gltf.meshes.size(); ++mesh_idx)
		{
			auto const& gltf_mesh = gltf.meshes[mesh_idx];
			auto& staging_mesh = staging.m_meshes[mesh_idx];
			staging_mesh.m_submesh_offset = u16(all_submesh_count);
			staging_mesh.m_submesh_count = u16(gltf_mesh.primitives.size());

			all_submesh_count += gltf_mesh.primitives.size();
		}

		staging.m_submeshes.resize(all_submesh_count);
	}

	void ProcessPrimitiveIndices(StagingModel& staging, tinygltf::Model const& gltf)
	{
		std::vector<uint32_t> acc_cache(gltf.accessors.size(), -1);
		size_t index_u32_count = 0;
		size_t index_u16_count = 0;
		for (int mesh_idx = 0; mesh_idx < gltf.meshes.size(); ++mesh_idx)
		{
			auto const& gltf_mesh = gltf.meshes[mesh_idx];
			for (int prim_idx = 0; prim_idx < gltf_mesh.primitives.size(); ++prim_idx)
			{
				auto const& gltf_prim = gltf_mesh.primitives[prim_idx];
				int acc_idx = gltf_prim.indices;
				if (acc_idx < 0) continue;
				auto const& acc = gltf.accessors[gltf_prim.indices];
				const int submesh_idx = staging.m_meshes[mesh_idx].m_submesh_offset + prim_idx;
				auto& submesh = staging.m_submeshes[submesh_idx];
				if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					if (acc_cache[acc_idx] == -1)
					{
						acc_cache[gltf_prim.indices] = u32(index_u32_count);
						index_u32_count += acc.count;
					}
					submesh.m_index_count = u32(acc.count);
					submesh.m_index_offset = u32(acc_cache[acc_idx]);
					submesh.m_index_stride = u32(sizeof(uint32_t));
				}
				else
				{
					if (acc_cache[acc_idx] == -1)
					{
						acc_cache[gltf_prim.indices] = u32(index_u16_count);
						index_u16_count += acc.count;
					}
					submesh.m_index_count = u32(acc.count);
					submesh.m_index_offset = u32(acc_cache[acc_idx]);
					submesh.m_index_stride = u32(sizeof(uint16_t));
				}
			}
		}

		staging.m_indices_u32.resize(index_u32_count);
		staging.m_indices_u16.resize(index_u16_count);

		for (size_t acc_idx = 0; acc_idx < acc_cache.size(); ++acc_idx)
		{
			const uint32_t offset = acc_cache[acc_idx];
			if (offset == -1) continue;
			auto const& acc = gltf.accessors[acc_idx];
			auto const& view = gltf.bufferViews[acc.bufferView];
			auto const& buf = gltf.buffers[view.buffer];
			if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				if (view.byteStride == 0)
				{
					std::memcpy(&staging.m_indices_u32[offset],
						buf.data.data() + view.byteOffset + acc.byteOffset,
						acc.count * sizeof(uint32_t));
					continue;
				}
				for (size_t i = 0; i < acc.count; ++i)
				{
					std::memcpy(
						&staging.m_indices_u32[offset + i],
						buf.data.data() + view.byteOffset + acc.byteOffset + i * view.byteStride,
						sizeof(uint32_t));
				}
			}
			else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				if (view.byteStride == 0)
				{
					std::memcpy(&staging.m_indices_u16[offset],
						buf.data.data() + view.byteOffset + acc.byteOffset,
						acc.count * sizeof(uint16_t));
					continue;
				}
				for (size_t i = 0; i < acc.count; ++i)
				{
					std::memcpy(
						&staging.m_indices_u16[offset + i],
						buf.data.data() + view.byteOffset + acc.byteOffset + i * view.byteStride,
						sizeof(uint16_t));
				}
			}
			else // if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			{
				size_t byte_stride = (view.byteStride != 0) ? view.byteStride : sizeof(uint8_t);
				for (size_t i = 0; i < acc.count; ++i)
				{
					staging.m_indices_u16[offset + i] = buf.data[view.byteOffset + acc.byteOffset + i * byte_stride];
				}
			}
		}
	}

	void ProcessPrimitivePositions(StagingModel& staging, tinygltf::Model const& gltf)
	{
		std::vector<uint32_t> acc_cache(gltf.accessors.size(), -1);
		size_t position_count = 0;
		for (size_t mesh_idx = 0; mesh_idx < gltf.meshes.size(); ++mesh_idx)
		{
			auto const& gltf_mesh = gltf.meshes[mesh_idx];
			for (size_t prim_idx = 0; prim_idx < gltf_mesh.primitives.size(); ++prim_idx)
			{
				auto const& gltf_prim = gltf_mesh.primitives[prim_idx];
				auto it = gltf_prim.attributes.find("POSITION");
				if (it == gltf_prim.attributes.end()) continue;
				int acc_idx = it->second;
				auto const& acc = gltf.accessors[acc_idx];
				const size_t submesh_idx = staging.m_meshes[mesh_idx].m_submesh_offset + prim_idx;
				auto& submesh = staging.m_submeshes[submesh_idx];
				if (acc_cache[acc_idx] == -1)
				{
					acc_cache[acc_idx] = u32(position_count);
					position_count += acc.count;
				}
				submesh.m_vertex_count = u32(acc.count);
				submesh.m_position_offset = u32(acc_cache[acc_idx]);
			}
		}

		staging.m_positions.resize(position_count);
		for (size_t acc_idx = 0; acc_idx < acc_cache.size(); ++acc_idx)
		{
			const uint32_t offset = acc_cache[acc_idx];
			if (offset == -1) continue;
			auto const& acc = gltf.accessors[acc_idx];
			auto const& view = gltf.bufferViews[acc.bufferView];
			auto const& buf = gltf.buffers[view.buffer];
			if (view.byteStride == 0)
			{
				std::memcpy(&staging.m_positions[offset],
					buf.data.data() + view.byteOffset + acc.byteOffset,
					acc.count * sizeof(XMFLOAT3));
				continue;
			}
			for (size_t i = 0; i < acc.count; ++i)
			{
				std::memcpy(
					&staging.m_positions[offset + i],
					buf.data.data() + view.byteOffset + acc.byteOffset + i * view.byteStride,
					sizeof(XMFLOAT3));
			}
		}
	}

	void ProcessPrimitiveNormals(StagingModel& staging, tinygltf::Model const& gltf)
	{
		std::vector<uint32_t> acc_cache(gltf.accessors.size(), -1);
		size_t normal_count = 0;
		for (size_t mesh_idx = 0; mesh_idx < gltf.meshes.size(); ++mesh_idx)
		{
			auto const& gltf_mesh = gltf.meshes[mesh_idx];
			for (size_t prim_idx = 0; prim_idx < gltf_mesh.primitives.size(); ++prim_idx)
			{
				auto const& gltf_prim = gltf_mesh.primitives[prim_idx];
				auto it = gltf_prim.attributes.find("NORMAL");
				if (it == gltf_prim.attributes.end()) continue;
				int acc_idx = it->second;
				auto const& acc = gltf.accessors[acc_idx];
				const size_t submesh_idx = staging.m_meshes[mesh_idx].m_submesh_offset + prim_idx;
				auto& submesh = staging.m_submeshes[submesh_idx];
				if (acc_cache[acc_idx] == -1)
				{
					acc_cache[acc_idx] = u32(normal_count);
					normal_count += acc.count;
				}
				submesh.m_normal_offset = u32(acc_cache[acc_idx]);
			}
		}
		staging.m_normals.resize(normal_count);
		for (size_t acc_idx = 0; acc_idx < acc_cache.size(); ++acc_idx)
		{
			const uint32_t offset = acc_cache[acc_idx];
			if (offset == -1) continue;
			auto const& acc = gltf.accessors[acc_idx];
			auto const& view = gltf.bufferViews[acc.bufferView];
			auto const& buf = gltf.buffers[view.buffer];
			if (view.byteStride == 0)
			{
				std::memcpy(&staging.m_normals[offset],
					buf.data.data() + view.byteOffset + acc.byteOffset,
					acc.count * sizeof(XMFLOAT3));
				continue;
			}
			for (size_t i = 0; i < acc.count; ++i)
			{
				std::memcpy(
					&staging.m_normals[offset + i],
					buf.data.data() + view.byteOffset + acc.byteOffset + i * view.byteStride,
					sizeof(XMFLOAT3));
			}
		}
	}

	void ProcessPrimitiveTexCoords(StagingModel& staging, tinygltf::Model const& gltf)
	{
		std::vector<uint32_t> acc_cache(gltf.accessors.size(), -1);
		size_t texcoord_count = 0;
		for (size_t mesh_idx = 0; mesh_idx < gltf.meshes.size(); ++mesh_idx)
		{
			auto const& gltf_mesh = gltf.meshes[mesh_idx];
			for (size_t prim_idx = 0; prim_idx < gltf_mesh.primitives.size(); ++prim_idx)
			{
				auto const& gltf_prim = gltf_mesh.primitives[prim_idx];
				auto it = gltf_prim.attributes.find("TEXCOORD_0");
				if (it == gltf_prim.attributes.end()) continue;
				int acc_idx = it->second;
				auto const& acc = gltf.accessors[acc_idx];
				const size_t submesh_idx = staging.m_meshes[mesh_idx].m_submesh_offset + prim_idx;
				auto& submesh = staging.m_submeshes[submesh_idx];
				if (acc_cache[acc_idx] == -1)
				{
					acc_cache[acc_idx] = u32(texcoord_count);
					texcoord_count += acc.count;
				}
				submesh.m_uv0_offset = u32(acc_cache[acc_idx]);
			}
		}
		staging.m_uv0s.resize(texcoord_count);
		for (size_t acc_idx = 0; acc_idx < acc_cache.size(); ++acc_idx)
		{
			const uint32_t offset = acc_cache[acc_idx];
			if (offset == -1) continue;
			auto const& acc = gltf.accessors[acc_idx];
			auto const& view = gltf.bufferViews[acc.bufferView];
			auto const& buf = gltf.buffers[view.buffer];
			if (view.byteStride == 0)
			{
				std::memcpy(&staging.m_uv0s[offset],
					buf.data.data() + view.byteOffset + acc.byteOffset,
					acc.count * sizeof(XMFLOAT2));
				continue;
			}
			for (size_t i = 0; i < acc.count; ++i)
			{
				std::memcpy(
					&staging.m_uv0s[offset + i],
					buf.data.data() + view.byteOffset + acc.byteOffset + i * view.byteStride,
					sizeof(XMFLOAT2));
			}
		}
	}

	void ProcessPrimitiveMaterials(StagingModel& staging, tinygltf::Model const& gltf)
	{
		for (size_t mesh_idx = 0; mesh_idx < gltf.meshes.size(); ++mesh_idx)
		{
			auto const& gltf_mesh = gltf.meshes[mesh_idx];
			for (size_t prim_idx = 0; prim_idx < gltf_mesh.primitives.size(); ++prim_idx)
			{
				auto const& gltf_prim = gltf_mesh.primitives[prim_idx];
				const size_t submesh_idx = staging.m_meshes[mesh_idx].m_submesh_offset + prim_idx;
				auto& submesh = staging.m_submeshes[submesh_idx];
				submesh.m_material_idx = gltf_prim.material;
			}
		}
	}

	void ProcessTextures(StagingModel& staging, tinygltf::Model const& gltf, std::string_view gltf_path)
	{
		staging.m_textures.reserve(gltf.images.size());
		for (auto const& image : gltf.images)
		{
			StagingTexture staging_texture = {};
			if (image.bufferView != -1)
			{
				auto const& view = gltf.bufferViews[image.bufferView];
				auto const& buf = gltf.buffers[view.buffer];
				staging_texture.m_bytes.resize(view.byteLength);
				std::memcpy(
					staging_texture.m_bytes.data(),
					buf.data.data() + view.byteOffset,
					staging_texture.m_bytes.size());
			}
			else if (image.uri.rfind("data:", 0) == 0)
			{
				size_t comma_pos = image.uri.find(',');
				staging_texture.m_bytes.resize(image.uri.size() - comma_pos - 1);
				std::memcpy(
					staging_texture.m_bytes.data(),
					image.uri.data() + comma_pos + 1,
					staging_texture.m_bytes.size());
			}
			else
			{
				std::filesystem::path gltf_base_dir = std::filesystem::path(gltf_path).parent_path();
				std::filesystem::path texture_path = gltf_base_dir / image.uri;
				staging_texture.m_path = texture_path.string();
			}
			staging.m_textures.push_back(std::move(staging_texture));
		}
	}

	void ProcessMaterials(StagingModel& staging, tinygltf::Model const& gltf)
	{
		staging.m_materials.reserve(gltf.materials.size());
		for (auto const& gltf_mat : gltf.materials)
		{
			StagingMaterial staging_mat = {};
			staging_mat.m_emissive_factor = XMFLOAT3(
				f32(gltf_mat.emissiveFactor[0]),
				f32(gltf_mat.emissiveFactor[1]),
				f32(gltf_mat.emissiveFactor[2]));
			const int base_color_tex = gltf_mat.pbrMetallicRoughness.baseColorTexture.index;
			if (base_color_tex != 0)
			{
				staging_mat.m_base_color_texture_idx = gltf.textures[base_color_tex].source;
			}
			staging_mat.m_metallic_factor = f32(gltf_mat.pbrMetallicRoughness.metallicFactor);
			staging_mat.m_roughness_factor = f32(gltf_mat.pbrMetallicRoughness.roughnessFactor);
			const int metallic_roughness_tex = gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (metallic_roughness_tex != 0)
			{
				staging_mat.m_metallic_roughness_texture_idx = gltf.textures[metallic_roughness_tex].source;
			}
			const int occlusion_tex = gltf_mat.occlusionTexture.index;
			if (occlusion_tex != 0)
			{
				staging_mat.m_occlusion_texture_idx = gltf.textures[occlusion_tex].source;
			}
			const int normal_tex = gltf_mat.normalTexture.index;
			if (normal_tex != 0)
			{
				staging_mat.m_normal_texture_idx = gltf.textures[normal_tex].source;
			}
			staging.m_materials.push_back(std::move(staging_mat));
		}
	}

	void ProcessMeshNodes(StagingModel& staging, tinygltf::Model const& gltf)
	{
		for (uint16_t node_idx = 0; node_idx < gltf.nodes.size(); ++node_idx)
		{
			auto const& node = gltf.nodes[node_idx];
			if (node.mesh < 0) continue;
			MeshInstance instance = {
				.m_node_idx = node_idx,
				.m_mesh_idx = u16(node.mesh)
			};
			staging.m_instances.push_back(instance);
		}
	}

	void ProcessNodeTransforms(StagingModel& staging, tinygltf::Model const& gltf)
	{
		staging.m_transforms = ComputeWorldTransforms(gltf);
	}
}
namespace rr
{
	StagingModel ModelLoader::LoadFromGLTF(std::string_view path)
	{
		tinygltf::Model gltf;
		tinygltf::TinyGLTF gltf_loader;
		std::string err, warn;

		bool ret = gltf_loader.LoadASCIIFromFile(&gltf, &err, &warn, std::string(path));
		if (!warn.empty()) OutputDebugStringA(warn.c_str());
		if (!err.empty())  OutputDebugStringA(err.c_str());
		if (!ret) throw std::runtime_error("Failed to load glTF file.");

		StagingModel m_model = {};
		ProcessMeshes(m_model, gltf);
		ProcessPrimitiveIndices(m_model, gltf);
		ProcessPrimitivePositions(m_model, gltf);
		ProcessPrimitiveNormals(m_model, gltf);
		ProcessPrimitiveTexCoords(m_model, gltf);
		ProcessPrimitiveMaterials(m_model, gltf);
		ProcessTextures(m_model, gltf, path);
		ProcessMeshNodes(m_model, gltf);
		ProcessNodeTransforms(m_model, gltf);

		ConvertWindingOrder(m_model.m_indices_u32);
		ConvertWindingOrder(m_model.m_indices_u16);
		ConvertRhToLhVector3(m_model.m_positions);
		ConvertRhToLhVector3(m_model.m_normals);
		ConvertRhToLhTransforms(m_model.m_transforms);

		return m_model;
	}
}

