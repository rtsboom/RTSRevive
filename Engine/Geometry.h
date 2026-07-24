#pragma once

namespace rr
{
	struct Geometry
	{
		int indices;
		int positions;
		int normals;
		int tangents;
		int uvs;
		int joints;
		int weights;
		int index_stride;
		int joint_stride;
	};
}