#include "pch.h"
#include <TinyGLTFv3/tiny_gltf_v3.h>
#include <cstdint>
#include <cstddef>
#include <span>

namespace
{
	using namespace rr;


}


namespace rr
{
	struct Material;
	struct MeshPrimitive;
	struct Mesh;
	struct SceneNode;
	struct AnimationClip;
	struct Skeleton;

	struct Model
	{
		std::span<std::byte>	 geometries; // gpu upload buffer directly
		std::span<Material>		 materials;
		std::span<MeshPrimitive> primitives;
		std::span<Mesh>			 meshes;
		std::span<SceneNode>	 nodes;
		std::span<AnimationClip> animclips;

		uint32_t skeleton;
	};

	struct MeshPrimitive
	{
		struct ByteSpan { uint32_t offset_bytes, size_bytes; };
		ByteSpan indices;
		uint32_t index_stride;

		// position stride is always 12B (float3)
		ByteSpan positions;

		// normal stride is always 4B (R8G8B8A8_SNORM)
		ByteSpan normals;

		// UV stride is always 4B (half2)
		ByteSpan uv0s;

		// tangent stride is always 4B (R8G8B8A8_SNORM)
		ByteSpan tangents;

		uint32_t material;
	};

}