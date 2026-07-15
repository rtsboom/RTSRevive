#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace rr
{
	template<typename T>
	struct ItemRange
	{
		uint32_t offset_bytes;
		uint32_t count;
		T* GetData(std::byte* blob) const
		{
			return reinterpret_cast<T*>(blob + offset_bytes);
		}

		std::span<T> GetSpan(std::byte* blob) const
		{
			return std::span<T>(GetData(blob), count);
		}
	};

	struct ByteRange
	{
		uint32_t offset_bytes;
		uint32_t size_bytes;
		std::span<std::byte> GetSpan(std::byte* blob) const
		{
			return std::span<std::byte>(blob + offset_bytes, size_bytes);
		}
	};


	struct Node
	{
		uint32_t parent;
		ItemRange<uint32_t> children;

		XMFLOAT3 translation;
		XMFLOAT4 rotation;
		XMFLOAT3 scale;
	};

	struct MeshInstance
	{
		uint32_t node;
		uint32_t primitive;
		int32_t  skin;
	};

	struct Primitive
	{
		ByteRange indices;
		ByteRange positions;
		ByteRange normals;
		ByteRange tangents;
		ByteRange uvs;
		ByteRange joints;
		ByteRange weights;
		uint32_t  index_stride;
		uint32_t  joint_stride;
	};

	struct Material
	{
		int base_color_texture{ -1 };
		int metallic_roughness_texture{ -1 };
		int normal_texture{ -1 };
		int occlusion_texture{ -1 };

		float base_color_factor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		float emissive_factor[3]{ 0.0f, 0.0f, 0.0f };
		float metallic_factor{ 0.0f };
		float roughness_factor{ 1.0f };
	};

	struct Animation
	{

	};

	struct Channel
	{

	};

	struct Skin
	{
		ItemRange<uint16_t>   joints;
		ItemRange<XMFLOAT4X4> inverse_bind_matrices;
	};

	struct Model
	{
		std::byte* data;
		uint32_t   data_size_bytes;

		ItemRange<Node>			nodes;
		ItemRange<MeshInstance>	instances;
		ItemRange<Primitive>	primitives;
		ItemRange<Material>		materials;
		ItemRange<Animation>	animations;
		ItemRange<Channel>		channels;
		ItemRange<Skin>			skins;

	};
}