#pragma once

#include <Common/AssetTypes.h>
#include <string_view>

namespace rrv::GLTFLoader
{
	struct Context
	{
		Asset::Model model;

		// CPU-side staging data; Ownership moves during upload
		
		std::vector<uint8_t>               geometry_data;
		std::vector<DirectX::ScratchImage> scratch_images;

		// uploaded to GPU but also kept on CPU.
		// on LH coordinate system.
		std::vector<XMFLOAT4X4> world_matrices;
	};

	Context Load(std::string_view path);
}
