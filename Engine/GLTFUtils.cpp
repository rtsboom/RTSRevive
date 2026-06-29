#include "pch.h"
#include <TinyGLTFv3/tiny_gltf_v3.h>
#include <cstdint>
#include <cstddef>

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
		std::byte* geometry_buffer;
		Material* materials;
		MeshPrimitive* primitives;
		Mesh* meshes;
		SceneNode* nodes;

		uint32_t geometry_buffer_size_bytes;
		uint32_t material_count;
		uint32_t primitive_count;
		uint32_t mesh_count;
		uint32_t node_count;

		AnimationClip* animclips;
		uint32_t animclip_count;

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