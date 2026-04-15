#pragma once

#include <string_view>
#include <string>
#include "AssetUtils.h"

namespace rr
{
	using DirectX::XMFLOAT4X4;

	struct StagingImage
	{
		std::string src;
		bool is_embedded;
	};
}
namespace rr::ModelLoader
{

	struct Context
	{
		Asset::Model model;

		// CPU-side staging data; Ownership moves during upload
		
		std::vector<uint8_t>      staging_buffer;
		std::vector<StagingImage> staging_images;

		// Uploaded to GPU but also kept on CPU; row major.
		std::vector<XMFLOAT4X4> world_matrices;
	};

	Context LoadFromGLTF(std::string_view path);
}
