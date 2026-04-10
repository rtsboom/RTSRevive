#include "pch.h"
#include "MeshGenerator.h"

MeshData MeshGenerator::CreateBox(float width, float height, float depth)
{
	float hw = width * 0.5f;
	float hh = height * 0.5f;
	float hd = depth * 0.5f;

	MeshData data;

	data.positions = {
		-hw, hh,-hd,  hw, hh,-hd,  hw, hh, hd, -hw, hh, hd,
		-hw,-hh, hd,  hw,-hh, hd,  hw,-hh,-hd, -hw,-hh,-hd,
		 hw,-hh, hd, -hw,-hh, hd, -hw, hh, hd,  hw, hh, hd,
		-hw,-hh,-hd,  hw,-hh,-hd,  hw, hh,-hd, -hw, hh,-hd,
		 hw,-hh,-hd,  hw,-hh, hd,  hw, hh, hd,  hw, hh,-hd,
		-hw,-hh, hd, -hw,-hh,-hd, -hw, hh,-hd, -hw, hh, hd,
	};

	//data.normals = {
	//	0,1,0,  0,1,0,  0,1,0,  0,1,0,
	//	0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0,
	//	0,0,1,  0,0,1,  0,0,1,  0,0,1,
	//	0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
	//	1,0,0,  1,0,0,  1,0,0,  1,0,0,
	//	-1,0,0, -1,0,0, -1,0,0, -1,0,0,
	//};

	//data.uvs = {
	//	0,0, 1,0, 1,1, 0,1,
	//	0,0, 1,0, 1,1, 0,1,
	//	0,0, 1,0, 1,1, 0,1,
	//	0,0, 1,0, 1,1, 0,1,
	//	0,0, 1,0, 1,1, 0,1,
	//	0,0, 1,0, 1,1, 0,1,
	//};

	for (uint32_t i = 0; i < 6; i++)
	{
		uint32_t base = i * 4;
		data.indices.insert(data.indices.end(), {
			base,     base + 2, base + 1,
			base,     base + 3, base + 2,
			});
	}

	return data;
}