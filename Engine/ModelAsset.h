#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace rr
{
	struct SceneNode
	{

	};

	struct Mesh
	{

	};

	struct MeshPrimitive
	{

	};

	struct Material
	{

	};

	struct AnimationClip
	{

	};

	template<typename T>
	struct BlobView
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

	struct ModelAsset
	{
		template<typename T>
		using View = BlobView<T>;

		size_t total_size_bytes;
		std::byte* blob;

		View<SceneNode>		scene_nodes;
		View<MeshPrimitive>	primitives;
		View<Mesh>			meshes;
		View<Material>		materials;
		View<AnimationClip> animations;
	};
}