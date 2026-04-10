#pragma once

namespace rrv
{
	using namespace DirectX;
	struct Geometry
	{
		enum class BufferSlot { POSITION, NORMAL, UV, INDEX, COUNT };
		static inline constexpr const char* attributeNames[]
		{
			"POSITION", "NORMAL", "TEXCOORD_0"
		};
		struct BufferView { uint32_t offset, stride; };

		uint32_t indexCount;
		uint32_t vertexCount;
		BufferView views[u64(BufferSlot::COUNT)];
	};

	struct Material
	{
		int   baseColorTextureIndex;
		float metallicFactor;
		float roughnessFactor;
		bool  hasBaseColorTexture;
		bool  hasMetallicFactor;
		bool  hasRoughnessFactor;
	};

	struct Mesh
	{
		uint32_t start;
		uint32_t count;
	};

	struct RenderItem
	{
		uint32_t meshIndex;
		uint32_t nodeIndex;
	};

	struct Model
	{
		//TODO: separate gpu resources 
		//GpuBuffer					geometryBuffer;
		//std::vector<GpuTexture>		textures;

		std::vector<Geometry>		geometries;
		std::vector<Material>		materials;
		std::vector<Mesh>			meshes;

		std::vector<RenderItem>		renderItems;
	};
}