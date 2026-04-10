#pragma once
#include "Mesh.h"
#include "RenderTypes.h"

namespace rrv::GltfLoader
{
	struct Context
	{
		rrv::Model model;

		// CPU-side byte data.
		// ownership moves to uploader; released on upload.
		std::vector<uint8_t>               geometry_data;
		std::vector<DirectX::ScratchImage> scratch_images;
		
		// uploaded to GPU but also kept on CPU.
		// on LH coordinate system.
		std::vector<XMFLOAT4X4> world_matrices;
	};

	Context Load(const std::string& path);
}

