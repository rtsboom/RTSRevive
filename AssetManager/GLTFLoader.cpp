#include "pch.h"
#include "GLTFLoader.h"
#include <MathUtils.h>


#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <filesystem>


namespace rr::ModelLoader
{

	static int GetAccessor(tinygltf::Primitive primitive)
	{

		return -1;
	}

	static int GetPrimitivePosition(tinygltf::Primitive primitive)
	{
		const auto it = primitive.attributes.find("POSITION");
		if (it != primitive.attributes.end()) return it->second;

		return -1;
	}

	static int GetPrimitiveNormal(tinygltf::Primitive primitive)
	{
		const auto it = primitive.attributes.find("NORMAL");
		if (it != primitive.attributes.end()) return it->second;
		return -1;
	}

	static int GetPrimitiveUV(tinygltf::Primitive primitive)
	{
		const auto it = primitive.attributes.find("TEXCOORD_0");
		if (it != primitive.attributes.end()) return it->second;
		return -1;
	}

	static XMMATRIX GetLocalMatrix(tinygltf::Node const& node)
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

	static std::vector<XMFLOAT4X4> ComputeWorldMatrices(tinygltf::Model const& gltf)
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

	static int GetElementSize(tinygltf::Accessor const& accessor)
	{
		return tinygltf::GetComponentSizeInBytes(accessor.componentType)
			* tinygltf::GetNumComponentsInType(accessor.type);
	}

	Context LoadFromGLTF(std::string_view path)
	{
		tinygltf::Model	gltf;
		tinygltf::TinyGLTF gltf_loader;
		std::string err, warn;
		bool ret = gltf_loader.LoadASCIIFromFile(&gltf, &err, &warn, std::string(path));
		if (!warn.empty()) OutputDebugStringA(warn.c_str());
		if (!err.empty())  OutputDebugStringA(err.c_str());
		if (!ret) return {};

		Context ctx = {};

		// staging image data; either embedded or file paths.
		ctx.staging_images.reserve(gltf.images.size());
		for (auto const& image : gltf.images)
		{
			StagingImage staging_image = {};

			if (image.bufferView != -1)
			{
				auto const& view = gltf.bufferViews[image.bufferView];
				auto const& buf = gltf.buffers[view.buffer];
				staging_image.src.assign(
					reinterpret_cast<const char*>(buf.data.data() + view.byteOffset),
					view.byteLength);

				staging_image.is_embedded = true;
			}
			else if (image.uri.rfind("data:", 0) == 0)
			{
				size_t comma_pos = image.uri.find(',');
				staging_image.src = image.uri.substr(comma_pos + 1);
				staging_image.is_embedded = true;
			}
			else
			{
				std::filesystem::path gltf_base_dir = std::filesystem::path(path).parent_path();
				std::filesystem::path image_path = gltf_base_dir / image.uri;
				staging_image.src = image_path.string();
			}
			ctx.staging_images.push_back(std::move(staging_image));
		}

		// Compute world matrices for all nodes in the scene.
		ctx.world_matrices = ComputeWorldMatrices(gltf);


		size_t primitive_count = 0;
		std::vector<uint32_t> flatten_mesh_offsets;
		flatten_mesh_offsets.reserve(gltf.meshes.size());

		for (auto const& gltf_mesh : gltf.meshes)
		{
			flatten_mesh_offsets.push_back(u32(primitive_count));
			primitive_count += gltf_mesh.primitives.size();
		}
		ctx.model.meshes.reserve(primitive_count);
		ctx.staging_buffer.reserve(64 * 1024);

		std::vector<int> acc_to_mesh_view(gltf.accessors.size(), -1);
		std::vector<rr::ByteSpan> mesh_views;
		mesh_views.reserve(gltf.accessors.size());

		for (auto const& gltf_mesh : gltf.meshes)
		{
			for (auto const& primitive : gltf_mesh.primitives)
			{
				int acc_idxs[u32(Asset::Mesh::View::COUNT)];
				std::fill_n(acc_idxs, _countof(acc_idxs), -1);
				acc_idxs[u32(Asset::Mesh::View::POSITION)] = GetPrimitivePosition(primitive);
				acc_idxs[u32(Asset::Mesh::View::NORMAL)] = GetPrimitiveNormal(primitive);
				acc_idxs[u32(Asset::Mesh::View::UV)] = GetPrimitiveUV(primitive);
				acc_idxs[u32(Asset::Mesh::View::INDEX)] = primitive.indices;


				Asset::Mesh flatten_mesh = {};

				if (primitive.indices != -1)
				{
					flatten_mesh.index_count = u32(gltf.accessors[primitive.indices].count);
				}
				if (const int pos_acc_idx = acc_idxs[u32(Asset::Mesh::View::POSITION)];
					pos_acc_idx != -1)
				{
					flatten_mesh.vertex_count = u32(gltf.accessors[pos_acc_idx].count);
				}

				for (int slot = 0; slot < _countof(acc_idxs); ++slot)
				{
					const int acc_idx = acc_idxs[slot];
					if (acc_idx == -1) continue;

					if (const int byte_span_idx = acc_to_mesh_view[acc_idx];
						byte_span_idx != -1)
					{
						flatten_mesh.views[slot] = mesh_views[byte_span_idx];
						continue;
					}


					auto const& acc = gltf.accessors[acc_idx];
					auto const& view = gltf.bufferViews[acc.bufferView];
					auto const& buf = gltf.buffers[view.buffer];
					uint8_t const* base = buf.data.data() + view.byteOffset + acc.byteOffset;

					const int elem_size = GetElementSize(acc);
					const size_t stride = view.byteStride ? view.byteStride : elem_size;

					rr::ByteSpan mesh_view = {};
					mesh_view.offset = u32(ctx.staging_buffer.size());
					mesh_view.length = u32(acc.count * elem_size);
					mesh_view.stride = u32(stride);
					
					acc_to_mesh_view[acc_idx] = i32(mesh_views.size());
					mesh_views.push_back(mesh_view);

					for (int i = 0; i < acc.count; ++i)
					{
						uint8_t const* elem_data = base + i * stride;
						ctx.staging_buffer.insert(ctx.staging_buffer.end(), elem_data, elem_data + elem_size);
					}

					ctx.staging_buffer.resize(rr::AlignUp<4>(ctx.staging_buffer.size()));
				}

				ctx.model.meshes.push_back(flatten_mesh);
			}
		}

		// Populate material metadata.
		for (auto& gltf_mesh : gltf.meshes)
		{
			for (auto& primitive : gltf_mesh.primitives)
			{
				Asset::Material material = {};
				if (primitive.material != -1)
				{
					auto& mat = gltf.materials[primitive.material];
					if (auto it = mat.values.find("baseColorTexture"); it != mat.values.end())
					{
						material.base_color_texture_idx = it->second.TextureIndex();
					}
					if (auto it = mat.values.find("metallicFactor"); it != mat.values.end())
					{
						material.metallic_factor = f32(it->second.Factor());
						material.has_metallic_factor = true;
					}
					if (auto it = mat.values.find("roughnessFactor"); it != mat.values.end())
					{
						material.roughness_factor = f32(it->second.Factor());
						material.has_roughness_factor = true;
					}
				}
				ctx.model.materials.push_back(material);
			}
		}

		for (int node_idx = 0; node_idx < gltf.nodes.size(); ++node_idx)
		{
			auto const& node = gltf.nodes[node_idx];
			if (node.mesh < 0) continue;

			for (int mesh_idx = flatten_mesh_offsets[node.mesh];
				mesh_idx < gltf.meshes[node.mesh].primitives.size(); ++mesh_idx)
			{
				Asset::MeshNode mesh_node = {};
				mesh_node.mesh_idx = node.mesh;
				mesh_node.node_idx = node_idx;
				ctx.model.mesh_nodes.push_back(mesh_node);
			}
		}

		return ctx;
	}

}