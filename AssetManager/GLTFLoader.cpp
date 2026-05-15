#include "pch.h"
#include "GLTFLoader.h"
#include "StagingModel.h"
#include "ModelAsset.h"
#include <CastUtils.h>

#pragma warning(push, 0)
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <TinyGLTFv3/tiny_gltf_v3.h>
#pragma warning(pop)

#include <DirectXMath.h>

#include <Windows.h>
#include <utility>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <memory>
#include <cstddef>
#include <string.h>
#include <algorithm>

namespace
{
	using namespace DirectX;
	using namespace DirectX::PackedVector;
	using namespace rr;

	XMMATRIX GetNodeLocalMatrix(tg3_node const& node)
	{
		if (node.has_matrix)
		{
			// glTF matrix is column-major.
			// Copying sequentially into row-major XMFLOAT implicitly transposes it.
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
		m *= XMMatrixScaling(
			f32(node.scale[0]),
			f32(node.scale[1]),
			f32(node.scale[2]));

		XMVECTOR q = XMVectorSet(
			f32(node.rotation[0]),
			f32(node.rotation[1]),
			f32(node.rotation[2]),
			f32(node.rotation[3]));
		m *= XMMatrixRotationQuaternion(q);

		m *= XMMatrixTranslation(
			f32(node.translation[0]),
			f32(node.translation[1]),
			f32(node.translation[2]));

		return m;
	}

	struct IndexExtractResult
	{
		uint32_t byte_offset = UINT32_MAX;
		uint32_t byte_stride = 0;
		uint32_t index_count = 0;
	};
	struct VertexExtractResult
	{
		uint32_t base_idx = UINT32_MAX;
		uint32_t vertex_count = 0;
	};

	struct SurfKey
	{
		int16_t normal = -1;
		int16_t tangent = -1;
		int16_t uv = -1;

		bool operator==(SurfKey const& other) const = default;
		bool IsNull() const { return normal < 0 && tangent < 0 && uv < 0; }
	};

	struct SkinKey
	{
		int16_t joint = -1;
		int16_t weight = -1;

		bool operator==(SkinKey const& other) const = default;
		bool IsNull() const { return joint < 0 && weight < 0; }
	};

	struct AccReader
	{
		tg3_accessor const& acc;
		tg3_buffer_view const& view;
		tg3_buffer const& buf;

		uint8_t const* src;
		size_t byte_stride;

		size_t Count() const noexcept { return acc.count; }
		size_t ByteCountContiguous() const noexcept { return acc.count * byte_stride; }

		template<typename T>
		T Read(size_t i) const
		{
			T v{};
			std::memcpy(&v, src + i * byte_stride, sizeof(T));
			return v;
		}

		void ReadAllContiguous(void* dst) const
		{
			std::memcpy(dst, src, ByteCountContiguous());
		}

		AccReader(tg3_model const* model, int32_t acc_idx)
			: acc(model->accessors[acc_idx])
			, view(model->buffer_views[acc.buffer_view])
			, buf(model->buffers[view.buffer])
			, src(buf.data.data + view.byte_offset + acc.byte_offset)
			, byte_stride(tg3_accessor_byte_stride(&acc, &view))
		{
			RR_CHECK(byte_stride > 0);
		}
	};

	class GltfImporter
	{
	public:
		explicit GltfImporter(tg3_model const* model) : model_(model) {}
		std::unique_ptr<AssetBase> Import()
		{
			ModelAsset out{};
			ExtractNodes(out);
			ExtractMeshes(out);
			ExtractMaterials(out);
			ExtractAnimations(out);

			return std::make_unique<ModelAsset>(std::move(out));
		}

		static int32_t FindAttribute(tg3_primitive const& prim, std::string_view name)
		{
			for (size_t i{}; i < prim.attributes_count; ++i)
			{
				std::string_view key(prim.attributes[i].key.data, prim.attributes[i].key.len);
				if (key == name) return prim.attributes[i].value;
			}

			return TG3_INDEX_NONE;
		}

	private:

		void BuildNodeHierarchy();
		void ExtractNodeMatrices(ModelAsset& out);
		void ExtractNodePrimitives(ModelAsset& out);
		void ExtractNodes(ModelAsset& out);

		IndexExtractResult ExtractIndices(tg3_primitive const& prim, std::vector<std::byte>& indices);
		VertexExtractResult ExtractPositions(tg3_primitive const& prim, std::vector<VertexPosition>& positions);
		VertexExtractResult ExtractSurfaces(tg3_primitive const& prim, std::vector<VertexSurface>& surfaces);
		VertexExtractResult ExtractSkins(tg3_primitive const& prim, std::vector<VertexSkin>& skins);

		void ExtractMeshes(ModelAsset& out);
		void ExtractMaterials(ModelAsset& out);

		struct Transform
		{
			XMFLOAT3 s;
			XMFLOAT4 r;
			XMFLOAT3 t;
			XMMATRIX MatrixScale() const { return XMMatrixScaling(s.x, s.y, s.z); }
			XMMATRIX MatrixRotation() const { return XMMatrixRotationQuaternion(XMLoadFloat4(&r)); }
			XMMATRIX MatrixTranslation() const { return XMMatrixTranslation(t.x, t.y, t.z); }
			XMMATRIX Matrix() const { return MatrixScale() * MatrixRotation() * MatrixTranslation(); }

		};
		void InitRestPoseTransforms(size_t frame_count, std::vector<Transform>& transforms);
		void ExtractAnimations(ModelAsset& out);

	private:
		tg3_model const* model_{};
		std::vector<int32_t> bfs_order_;
		std::vector<int32_t> node_parent_;

		std::vector<std::pair<int16_t, IndexExtractResult>> indices_cache_;
		std::vector<std::pair<int16_t, VertexExtractResult>> pos_cache_;
		std::vector<std::pair<SurfKey, VertexExtractResult>> surf_cache_;
		std::vector<std::pair<SkinKey, VertexExtractResult>> skin_cache_;
	};

	IndexExtractResult GltfImporter::ExtractIndices(tg3_primitive const& prim, std::vector<std::byte>& indices)
	{
		int32_t const acc_idx = prim.indices;
		if (acc_idx < 0) return {};

		int16_t const key = i16(acc_idx);

		for (size_t i{}; i < indices_cache_.size(); ++i)
		{
			if (indices_cache_[i].first == key) return indices_cache_[i].second;
		}

		AccReader const reader(model_, acc_idx);
		size_t const byte_offset = indices.size();
		size_t const byte_count = reader.ByteCountContiguous();
		indices.resize(byte_offset + byte_count);

		reader.ReadAllContiguous(indices.data() + byte_offset);

		IndexExtractResult const result = {
			.byte_offset = u32(byte_offset),
			.byte_stride = u32(reader.byte_stride),
			.index_count = u32(reader.Count())
		};
		indices_cache_.push_back({ key, result });
		return result;
	}

	VertexExtractResult GltfImporter::ExtractPositions(tg3_primitive const& prim, std::vector<VertexPosition>& positions)
	{
		int32_t const acc_idx = FindAttribute(prim, "POSITION");
		RR_CHECK(acc_idx >= 0);

		int16_t const key = i16(acc_idx);
		for (size_t i{}; i < pos_cache_.size(); ++i)
		{
			if (pos_cache_[i].first == key) return pos_cache_[i].second;
		}

		AccReader const reader(model_, acc_idx);
		size_t const base_idx = positions.size();
		size_t const vertex_count = reader.Count();

		positions.resize(base_idx + vertex_count);
		for (size_t i{}; i < vertex_count; ++i)
		{
			positions[base_idx + i] = reader.Read<XMFLOAT3>(i);
		}

		VertexExtractResult const result =
		{
			.base_idx = u32(base_idx),
			.vertex_count = u32(vertex_count)
		};
		pos_cache_.push_back({ key, result });
		return result;
	}

	VertexExtractResult GltfImporter::ExtractSurfaces(tg3_primitive const& prim, std::vector<VertexSurface>& surfaces)
	{
		int32_t const normal_acc_idx = FindAttribute(prim, "NORMAL");
		int32_t const tangent_acc_idx = FindAttribute(prim, "TANGENT");
		int32_t const uv_acc_idx = FindAttribute(prim, "TEXCOORD_0");
		SurfKey const key =
		{
			.normal = i16(normal_acc_idx),
			.tangent = i16(tangent_acc_idx),
			.uv = i16(uv_acc_idx)
		};

		if (key.IsNull()) return {};

		for (size_t i{}; i < surf_cache_.size(); ++i)
		{
			if (surf_cache_[i].first == key) return surf_cache_[i].second;
		}

		size_t const base_idx = surfaces.size();
		size_t vertex_count = 0;

		if (normal_acc_idx >= 0)
		{
			AccReader const normal_reader(model_, normal_acc_idx);
			if (vertex_count == 0)
			{
				vertex_count = normal_reader.Count();
				surfaces.resize(base_idx + vertex_count);
			}

			RR_CHECK(vertex_count == normal_reader.Count());
			for (size_t i{}; i < normal_reader.Count(); ++i)
			{
				XMFLOAT3 n = normal_reader.Read<XMFLOAT3>(i);
				XMVECTOR v = XMLoadFloat3(&n);
				XMStoreByteN4(&surfaces[base_idx + i].normal, v);
			}
		}

		if (tangent_acc_idx >= 0)
		{
			AccReader const tangent_reader(model_, tangent_acc_idx);
			if (vertex_count == 0)
			{
				vertex_count = tangent_reader.Count();
				surfaces.resize(base_idx + vertex_count);
			}
			RR_CHECK(vertex_count == tangent_reader.Count());
			for (size_t i{}; i < tangent_reader.Count(); ++i)
			{
				XMFLOAT4 t = tangent_reader.Read<XMFLOAT4>(i);
				XMVECTOR v = XMLoadFloat4(&t);
				XMStoreByteN4(&surfaces[base_idx + i].tangent, v);
			}
		}

		if (uv_acc_idx >= 0)
		{
			AccReader const uv_reader(model_, uv_acc_idx);
			if (vertex_count == 0)
			{
				vertex_count = uv_reader.Count();
				surfaces.resize(base_idx + vertex_count);
			}
			RR_CHECK(vertex_count == uv_reader.Count());

			for (size_t i{}; i < uv_reader.Count(); ++i)
			{
				surfaces[base_idx + i].uv = uv_reader.Read<XMFLOAT2>(i);
			}
		}

		VertexExtractResult const result =
		{
			.base_idx = u32(base_idx),
			.vertex_count = u32(vertex_count)
		};
		surf_cache_.push_back({ key, result });
		return result;
	}

	VertexExtractResult GltfImporter::ExtractSkins(tg3_primitive const& prim, std::vector<VertexSkin>& skins)
	{
		int32_t const joint_acc_idx = FindAttribute(prim, "JOINTS_0");
		int32_t const weight_acc_idx = FindAttribute(prim, "WEIGHTS_0");
		SkinKey const key =
		{
			.joint = i16(joint_acc_idx),
			.weight = i16(weight_acc_idx),
		};

		if (key.IsNull()) return {};

		for (size_t i{}; i < skin_cache_.size(); ++i)
		{
			if (skin_cache_[i].first == key) return skin_cache_[i].second;
		}

		AccReader const joint_reader(model_, joint_acc_idx);
		AccReader const weight_reader(model_, weight_acc_idx);
		size_t const base_idx = skins.size();
		size_t const vertex_count = joint_reader.Count();
		skins.resize(base_idx + vertex_count);

		if (joint_reader.acc.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			for (size_t i{}; i < vertex_count; ++i)
			{
				XMUSHORT4 j = joint_reader.Read<XMUSHORT4>(i);
				XMVECTOR v = XMLoadUShort4(&j);

				XMStoreUByte4(&skins[base_idx + i].joint, v);
			}
		}
		else if (joint_reader.acc.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			for (size_t i{}; i < vertex_count; ++i)
			{
				skins[base_idx + i].joint = joint_reader.Read<XMUBYTE4>(i);
			}
		}
		else
		{
			RR_CHECK(false && "Unsupported skin joint component type");
		}

		// remap joint to node
		for (size_t i{}; i < vertex_count; ++i)
		{
			auto& s = skins[base_idx + i];
			auto const& joints = model_->skins[0].joints;
			s.joint.x = u8(joints[s.joint.x]);
			s.joint.y = u8(joints[s.joint.y]);
			s.joint.z = u8(joints[s.joint.z]);
			s.joint.w = u8(joints[s.joint.w]);
		}

		if (weight_reader.acc.component_type == TG3_COMPONENT_TYPE_FLOAT)
		{
			for (size_t i{}; i < vertex_count; ++i)
			{
				XMFLOAT4 w = weight_reader.Read<XMFLOAT4>(i);
				XMVECTOR v = XMLoadFloat4(&w);
				XMStoreUByteN4(&skins[base_idx + i].weight, v);
			}
		}
		else if (weight_reader.acc.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			for (size_t i{}; i < vertex_count; ++i)
			{
				XMUSHORTN4 w = weight_reader.Read<XMUSHORTN4>(i);
				XMVECTOR v = XMLoadUShortN4(&w);
				XMStoreUByteN4(&skins[base_idx + i].weight, v);
			}
		}
		else if (weight_reader.acc.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			for (size_t i{}; i < vertex_count; ++i)
			{
				XMUBYTEN4 w = weight_reader.Read<XMUBYTEN4>(i);
				skins[base_idx + i].weight = w;
			}
		}
		else
		{
			RR_CHECK(false && "Unsupported skin weight component type");
		}

		VertexExtractResult const result =
		{
			.base_idx = u32(base_idx),
			.vertex_count = u32(vertex_count)
		};
		skin_cache_.push_back({ key, result });
		return result;
	}

	void GltfImporter::BuildNodeHierarchy()
	{
		node_parent_.resize(model_->nodes_count, -1);
		bfs_order_.reserve(model_->nodes_count);

		std::vector<int32_t> q;
		q.reserve(model_->nodes_count);
		size_t q_head{};

		tg3_scene const& scene = model_->scenes[0];
		for (size_t i{}; i < scene.nodes_count; ++i)
		{
			q.push_back(scene.nodes[i]);
		}

		while (q_head < q.size())
		{
			int32_t const node_idx = q[q_head++];
			bfs_order_.push_back(node_idx);
			tg3_node const& node = model_->nodes[node_idx];
			for (size_t i{}; i < node.children_count; ++i)
			{
				q.push_back(node.children[i]);
				node_parent_[q.back()] = node_idx;
			}
		}
	}

	void GltfImporter::ExtractNodeMatrices(ModelAsset& out)
	{
		out.node_matrices.resize(model_->nodes_count);
		for (size_t node_idx : bfs_order_)
		{
			tg3_node const& node = model_->nodes[node_idx];
			int32_t const parent_idx = node_parent_[node_idx];
			XMMATRIX const local_mat = GetNodeLocalMatrix(node);
			XMMATRIX const world_mat = (parent_idx >= 0)
				? local_mat * XMLoadFloat4x4(&out.node_matrices[parent_idx])
				: local_mat;

			XMStoreFloat4x4(&out.node_matrices[node_idx], world_mat);
		}
	}

	void GltfImporter::ExtractNodePrimitives(ModelAsset& out)
	{
		std::vector<uint32_t> mesh_prim_offset;
		mesh_prim_offset.reserve(model_->meshes_count + 1); // +1 to store total primitive count at the end
		mesh_prim_offset.push_back(0);
		for (uint32_t i{}; i < model_->meshes_count; ++i)
		{
			mesh_prim_offset.push_back(mesh_prim_offset.back() + model_->meshes[i].primitives_count);
		}

		out.node_primitives.reserve(mesh_prim_offset.back());

		for (uint32_t i{}; i < model_->nodes_count; ++i)
		{
			tg3_node const& node = model_->nodes[i];
			if (node.mesh >= 0)
			{
				for (uint32_t j{}; j < model_->meshes[node.mesh].primitives_count; ++j)
				{
					NodePrimitive const node_prim = {
						.node_idx = u16(i),
						.primitive_idx = u16(mesh_prim_offset[node.mesh] + j)
					};

					out.node_primitives.push_back(node_prim);
				}
			}
		}
	}

	void GltfImporter::ExtractNodes(ModelAsset& out)
	{
		BuildNodeHierarchy();
		ExtractNodeMatrices(out);
		ExtractNodePrimitives(out);
	}

	void GltfImporter::ExtractMeshes(ModelAsset& out)
	{
		for (size_t mesh_idx{}; mesh_idx < model_->meshes_count; ++mesh_idx)
		{
			tg3_mesh const& mesh = model_->meshes[mesh_idx];
			for (size_t prim_idx{}; prim_idx < mesh.primitives_count; ++prim_idx)
			{
				tg3_primitive const& prim = mesh.primitives[prim_idx];

				Primitive out_prim = {};
				auto idx_result = ExtractIndices(prim, out.indices);
				out_prim.index_byte_offset = idx_result.byte_offset;
				out_prim.index_byte_stride = idx_result.byte_stride;
				out_prim.index_count = idx_result.index_count;

				auto pos_result = ExtractPositions(prim, out.positions);
				out_prim.position_base_idx = pos_result.base_idx;
				out_prim.vertex_count = pos_result.vertex_count;

				auto surf_result = ExtractSurfaces(prim, out.surfaces);
				out_prim.surface_base_idx = surf_result.base_idx;

				auto skin_result = ExtractSkins(prim, out.skins);
				out_prim.skin_base_idx = skin_result.base_idx;

				RR_CHECK(surf_result.vertex_count == 0 || surf_result.vertex_count == out_prim.vertex_count);
				RR_CHECK(skin_result.vertex_count == 0 || skin_result.vertex_count == out_prim.vertex_count);

				out_prim.material_idx = prim.material;

				out.primitives.push_back(out_prim);
			}
		}
	}

	void GltfImporter::ExtractMaterials(ModelAsset& out)
	{
		out.materials.resize(model_->materials_count);
		for (size_t i{}; i < out.materials.size(); ++i)
		{
			tg3_material const& mat = model_->materials[i];

			Material out_mat = {};

			const int base_color_tex = mat.pbr_metallic_roughness.base_color_texture.index;
			if (base_color_tex >= 0)
			{
				out_mat.base_color_texture_idx = model_->textures[base_color_tex].source;
			}
			const int metallic_roughness_tex = mat.pbr_metallic_roughness.metallic_roughness_texture.index;
			if (metallic_roughness_tex >= 0)
			{
				out_mat.metallic_roughness_texture_idx = model_->textures[metallic_roughness_tex].source;
			}

			const int normal_tex = mat.normal_texture.index;
			if (normal_tex >= 0)
			{
				out_mat.normal_texture_idx = model_->textures[normal_tex].source;
			}

			const int occlusion_tex = mat.occlusion_texture.index;
			if (occlusion_tex >= 0)
			{
				out_mat.occlusion_texture_idx = model_->textures[occlusion_tex].source;
			}

			out_mat.base_color_factor = XMFLOAT4(
				f32(mat.pbr_metallic_roughness.base_color_factor[0]),
				f32(mat.pbr_metallic_roughness.base_color_factor[1]),
				f32(mat.pbr_metallic_roughness.base_color_factor[2]),
				f32(mat.pbr_metallic_roughness.base_color_factor[3]));

			out_mat.emissive_factor = XMFLOAT3(
				f32(mat.emissive_factor[0]),
				f32(mat.emissive_factor[1]),
				f32(mat.emissive_factor[2]));

			out_mat.metallic_factor = f32(mat.pbr_metallic_roughness.metallic_factor);
			out_mat.roughness_factor = f32(mat.pbr_metallic_roughness.roughness_factor);
			out.materials[i] = out_mat;

		}
	}

	void GltfImporter::InitRestPoseTransforms(size_t frame_count, std::vector<Transform>& transforms)
	{
		size_t const node_count = model_->nodes_count;

		transforms.resize(frame_count * node_count);
		for (size_t node_i{}; node_i < node_count; ++node_i)
		{
			auto const& node = model_->nodes[node_i];
			Transform const transform = {
				.s = XMFLOAT3(f32(node.scale[0]), f32(node.scale[1]), f32(node.scale[2])),
				.r = XMFLOAT4(f32(node.rotation[0]), f32(node.rotation[1]), f32(node.rotation[2]), f32(node.rotation[3])),
				.t = XMFLOAT3(f32(node.translation[0]), f32(node.translation[1]), f32(node.translation[2]))
			};

			transforms[node_i] = transform;
		}

		for (size_t frame_i{ 1 }; frame_i < frame_count; ++frame_i)
		{
			std::copy(
				transforms.begin(),
				transforms.begin() + node_count,
				transforms.begin() + frame_i * node_count);
		}
	}

	void GltfImporter::ExtractAnimations(ModelAsset& out)
	{
		size_t const node_count = model_->nodes_count;
		XMFLOAT4X4 const* inv_bind_matrices = nullptr;
		if (model_->skins_count > 0)
		{
			tg3_skin const& skin = model_->skins[0];
			int32_t const ibm_acc_idx = skin.inverse_bind_matrices;
			if (ibm_acc_idx >= 0)
			{
				AccReader const ibm_reader(model_, ibm_acc_idx);
				inv_bind_matrices = reinterpret_cast<XMFLOAT4X4 const*>(ibm_reader.src);
			}
		}

		size_t max_frame_count = 0;
		size_t matrix_base_idx = out.node_matrices.size();
		for (size_t i{}; i < model_->animations_count; ++i)
		{
			auto const& anim = model_->animations[i];
			RR_CHECK(anim.samplers_count > 0);
			auto const& sampler = anim.samplers[0];
			AccReader const sampler_in_reader(model_, sampler.input);
			float const* timestamps = reinterpret_cast<float const*>(sampler_in_reader.src);

			AnimationClip out_clip = {};
			out_clip.name = std::string(anim.name.data, anim.name.len);
			out_clip.duration = timestamps[sampler_in_reader.Count() - 1];
			out_clip.frame_count = u32(sampler_in_reader.Count());
			out_clip.node_count = u32(node_count);
			out_clip.node_matrix_base_idx = u32(matrix_base_idx);
			matrix_base_idx += sz(out_clip.frame_count) * out_clip.node_count;
			max_frame_count = std::max(max_frame_count, sz(out_clip.frame_count));
		}
		out.node_matrices.resize(matrix_base_idx);

		std::vector<Transform> transforms;
		for (size_t i{}; i < model_->animations_count; ++i)
		{
			InitRestPoseTransforms(max_frame_count, transforms);
			auto const& anim = model_->animations[i];
			auto const& clip = out.animations[i];
			for (uint32_t ch_idx{}; ch_idx < anim.channels_count; ++ch_idx)
			{
				auto const& channel = anim.channels[ch_idx];
				auto const& sampler = anim.samplers[channel.sampler];
				auto const& sampler_in_reader = AccReader(model_, sampler.input);
				auto const& sampler_out_reader = AccReader(model_, sampler.output);


				RR_CHECK(channel.target.node >= 0);
				if (tg3_str_equals_cstr(channel.target.path, "scale"))
				{
					XMFLOAT3 const* scales = reinterpret_cast<XMFLOAT3 const*>(sampler_out_reader.src);
					for (size_t frame_i{}; frame_i < clip.frame_count; ++frame_i)
					{
						transforms[frame_i * clip.node_count + channel.target.node].s = scales[frame_i];
					}
				}

				if (tg3_str_equals_cstr(channel.target.path, "rotation"))
				{
					XMFLOAT4 const* rotations = reinterpret_cast<XMFLOAT4 const*>(sampler_out_reader.src);
					for (size_t frame_i{}; frame_i < clip.frame_count; ++frame_i)
					{
						transforms[frame_i * clip.node_count + channel.target.node].r = rotations[frame_i];
					}
				}

				if (tg3_str_equals_cstr(channel.target.path, "translation"))
				{
					XMFLOAT3 const* translations = reinterpret_cast<XMFLOAT3 const*>(sampler_out_reader.src);
					for (size_t frame_i{}; frame_i < clip.frame_count; ++frame_i)
					{
						transforms[frame_i * clip.node_count + channel.target.node].t = translations[frame_i];
					}
				}
			}

			for (size_t frame_i{}; frame_i < clip.frame_count; ++frame_i)
			{
				size_t const frame_matrix_base_idx = clip.node_matrix_base_idx + frame_i * node_count;
				for (size_t node_i : bfs_order_)
				{
					int32_t const parent_i = node_parent_[node_i];
					XMMATRIX M = transforms[frame_i * node_count + node_i].Matrix();
					if (parent_i >= 0)
					{
						M *= XMLoadFloat4x4(&out.node_matrices[frame_matrix_base_idx + parent_i]);
					}
					XMLoadFloat4x4(&out.node_matrices[frame_matrix_base_idx + node_i]);
				}

				auto const& skin = model_->skins[0];
				for (size_t joint_i{}; joint_i < skin.joints_count; ++joint_i)
				{
					XMMATRIX M = XMMatrixIdentity();
					if (inv_bind_matrices) M = XMLoadFloat4x4(&inv_bind_matrices[joint_i]);

					size_t const node_i = skin.joints[joint_i];
					XMFLOAT4X4& out_matrix = out.node_matrices[frame_matrix_base_idx + node_i];
					M *= XMLoadFloat4x4(&out_matrix);
					XMStoreFloat4x4(&out_matrix, M);
				}
			}
		}
	}

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
	std::unique_ptr<AssetBase> ImportGLTF(std::string const& filename)
	{
		tg3_parse_options opts;
		tg3_parse_options_init(&opts);
		opts.images_as_is = 1; // Don't decode images
		opts.image = {}; // Don't load external images
		tg3_error_stack errors;
		tg3_model model;
		tg3_error_code err = tg3_parse_file(&model, &errors, filename.data(), u32(filename.size()), &opts);

		GltfImporter importer{ &model };

		tg3_model_free(&model);
		tg3_error_stack_free(&errors);
		return std::unique_ptr<AssetBase>();
	}
}
