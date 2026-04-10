#include "pch.h"
#include "GltfLoader.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>


namespace rrv::GltfLoader
{
	static int GetPrimitiveAccessor(Geometry::BufferSlot slot, tinygltf::Primitive primitive)
	{
		if (slot == Geometry::BufferSlot::INDEX)
			return primitive.indices;

		const auto it = primitive.attributes.find(
			Geometry::attributeNames[static_cast<size_t>(slot)]);
		
		if (it != primitive.attributes.end())
			return it->second;

		return -1;
	}

	static bool LoadImageDirectXTex(tinygltf::Image* image, const int imageIndex,
		std::string* err, std::string* warn,
		int reqWidth, int reqHeight,
		const unsigned char* bytes, int size, void* userData)
	{
		auto* ctx = static_cast<GltfLoader::Context*>(userData);

		DirectX::ScratchImage scratch;
		HRESULT hr;

		if (bytes)
		{
			hr = DirectX::LoadFromWICMemory(bytes, size, DirectX::WIC_FLAGS_NONE, nullptr, scratch);
		}
		else
		{
			std::wstring image_path(image->uri.begin(), image->uri.end());
			hr = DirectX::LoadFromWICFile(image_path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, scratch);
		}

		if (FAILED(hr))
		{
			if (err) *err = "Failed to load image: " + image->uri;
			return false;
		}

		*image = {};
		ctx->scratch_images.push_back(std::move(scratch));
		return true;
	}

	static XMMATRIX FlipZ()
	{
		return XMMatrixScaling(1.f, 1.f, -1.f);
	}

	static XMMATRIX GetLocalMatrix(const tinygltf::Node& node)
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

	static std::vector<XMFLOAT4X4> ComputeWorldMatrices(const tinygltf::Model& gltf)
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

		// Assumes vertex are in LH coordinate system,
		for (uint32_t i = 0; i < node_count; ++i)
		{
			XMMATRIX m = XMLoadFloat4x4(&out_matrices[i]);
			m *= FlipZ();
			XMStoreFloat4x4(&out_matrices[i], m);
		}

		return out_matrices;
	}

	static uint64_t ComputeGeometryDataSizeInBytes(tinygltf::Model& gltf)
	{
		uint64_t geometry_data_size = 0;
		for (auto& gltf_mesh : gltf.meshes)
		{
			for (auto& primitive : gltf_mesh.primitives)
			{
				for (int slot = 0; slot < i32(Geometry::BufferSlot::COUNT); ++slot)
				{
					int acc_idx = GetPrimitiveAccessor(static_cast<Geometry::BufferSlot>(slot), primitive);
					if (acc_idx == -1) continue;

					auto& acc = gltf.accessors[acc_idx];
					const int comp_type = acc.componentType;
					const int elem_size = tinygltf::GetComponentSizeInBytes(comp_type)
						* tinygltf::GetNumComponentsInType(comp_type);

					geometry_data_size += acc.count * elem_size;
					geometry_data_size = rrv::AlignUp(geometry_data_size, 4u);
				}
			}
		}

		return geometry_data_size;
	}

	Context Load(const std::string& path)
	{
		Context ctx;

		tinygltf::Model	gltf;
		tinygltf::TinyGLTF loader;
		std::string err, warn;
		loader.SetImageLoader(LoadImageDirectXTex, &ctx);

		bool ret = loader.LoadASCIIFromFile(&gltf, &err, &warn, path);
		if (!warn.empty()) OutputDebugStringA(warn.c_str());
		if (!err.empty())  OutputDebugStringA(err.c_str());
		if (!ret) return {};

		ctx.world_matrices = ComputeWorldMatrices(gltf);
		ctx.model.meshes.reserve(gltf.meshes.size());

		uint32_t primitive_count = 0;
		for (auto& gltf_mesh : gltf.meshes)
		{
			Mesh mesh = {};
			mesh.start = primitive_count;
			mesh.count = u32(gltf_mesh.primitives.size());
			ctx.model.meshes.push_back(mesh);

			primitive_count += mesh.count;
		}

		ctx.model.geometries.reserve(primitive_count);
		ctx.model.materials.reserve(primitive_count);

		const uint64_t geometry_data_size = ComputeGeometryDataSizeInBytes(gltf);
		ctx.geometry_data.reserve(geometry_data_size);


		for (auto& gltf_mesh : gltf.meshes)
		{
			for (auto& primitive : gltf_mesh.primitives)
			{
				Geometry geometry = {};

				// vertex attributes metadata
				for (int slot = 0; slot < int(Geometry::BufferSlot::COUNT); ++slot)
				{
					int acc_idx = GetPrimitiveAccessor(static_cast<Geometry::BufferSlot>(slot), primitive);
					if (acc_idx == -1) continue;


					auto& acc = gltf.accessors[acc_idx];
					auto& view = gltf.bufferViews[acc.bufferView];
					auto& buf = gltf.buffers[view.buffer];
					const uint8_t* base = buf.data.data() + view.byteOffset + acc.byteOffset;
					const int comp_type = acc.componentType;
					const int elem_size = tinygltf::GetComponentSizeInBytes(comp_type)
						* tinygltf::GetNumComponentsInType(comp_type);
					const size_t stride = view.byteStride ? view.byteStride : elem_size;


					geometry.views[slot].stride = u32(stride);
					geometry.views[slot].offset = u32(ctx.geometry_data.size());
					for (int i = 0; i < acc.count; ++i)
					{
						const uint8_t* elem_data = base + i * stride;
						ctx.geometry_data.insert(ctx.geometry_data.end(), elem_data, elem_data + elem_size);
					}

					ctx.geometry_data.resize(rrv::AlignUp(ctx.geometry_data.size(), 4u));
				}

				ctx.model.geometries.push_back(geometry);
			}
		}

		for (auto& gltf_mesh : gltf.meshes)
		{
			for (auto& primitive : gltf_mesh.primitives)
			{
				// materials metadata
				Material material = {};
				if (primitive.material != -1)
				{
					auto& mat = gltf.materials[primitive.material];
					if (auto it = mat.values.find("baseColorTexture"); it != mat.values.end())
					{
						material.baseColorTextureIndex = it->second.TextureIndex();
						material.hasBaseColorTexture = true;
					}
					if (auto it = mat.values.find("metallicFactor"); it != mat.values.end())
					{
						material.metallicFactor = f32(it->second.Factor());
						material.hasMetallicFactor = true;
					}
					if (auto it = mat.values.find("roughnessFactor"); it != mat.values.end())
					{
						material.roughnessFactor = f32(it->second.Factor());
						material.hasRoughnessFactor = true;
					}
				}
				ctx.model.materials.push_back(material);
			}
		}


		return ctx;
	}

}
