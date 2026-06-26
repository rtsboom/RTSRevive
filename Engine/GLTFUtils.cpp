#include "pch.h"
#include <TinyGLTFv3/tiny_gltf_v3.h>
#include <cstdint>

namespace
{
	using namespace rr;


}


namespace rr
{
	struct Model
	{
		uint32_t geometry_base_offset; // in bytes
		uint32_t geometry_total_bytes;

		uint32_t material_base_index;
		uint32_t material_count;
		uint32_t primitive_base_index;
		uint32_t primitive_count;
		uint32_t mesh_base_index;
		uint32_t mesh_count;
		uint32_t node_base_index;
		uint32_t node_count;

		uint32_t skeleton_index;
		uint32_t animation_clip_base_index;
		uint32_t animation_clip_count;
	};

	struct MeshPrimitive
	{
		uint32_t index_offset; // in bytes
		uint32_t index_stride;
		uint32_t index_count;

		// position stride is always 12B (float3)
		uint32_t position_offset; // in bytes
		uint32_t position_count;

		// normal stride is always 4B (R8G8B8A8_SNORM)
		uint32_t normal_offset; // in bytes
		uint32_t normal_count;

		// UV stride is always 4B (half2)
		uint32_t uv0_offset; // in bytes
		uint32_t uv0_count;

		// tangent stride is always 4B (R8G8B8A8_SNORM)
		uint32_t tangent_offset; // in bytes
		uint32_t tangent_count;

		uint32_t material_index;
	};

}