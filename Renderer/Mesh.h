#pragma once
#include <vector>
#include <cstdint>

struct TextureData
{
	std::vector<uint8_t> pixels;
	int width = 0;
	int height = 0;
};

struct VertexNormalUV
{
	float nx, ny, nz;  // normal
	float u, v;  // uv
};

struct MeshData
{
	std::vector<float> positions;
	//std::vector<float> normals;
	//std::vector<float> uvs;
	std::vector<VertexNormalUV> normalUVs;

	std::vector<uint32_t> indices;

	TextureData           albedo;
};
