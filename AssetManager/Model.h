#pragma once
#include <cstdint>
#include <vector>
#include <DirectXMath.h>
namespace rr
{
	struct Primitive
	{
		uint32_t m_index_offset;
		uint32_t m_index_stride;
		uint32_t m_index_count;

		uint32_t m_position_offset;
		uint32_t m_position_stride;

		uint32_t m_normal_offset;
		uint32_t m_normal_stride;

		uint32_t m_texcoord_offset;
		uint32_t m_texcoord_stride;

		uint32_t m_material;
	};

	struct Mesh
	{

	};

	struct Model
	{
		std::vector<Primitive> m_primitives;
		std::vector<DirectX::XMFLOAT4X4> m_node_transforms;
		struct Draw
		{
			uint16_t m_node_idx;
			uint16_t m_primitive_idx;
		};
		std::vector<Draw> m_draw;
	};
}