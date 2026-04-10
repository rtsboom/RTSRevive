#include "pch.h"
#include "GpuTextureRegistry.h"

namespace rrv
{
	static DirectX::ScratchImage LoadImageFromFile(std::string_view path)
	{

	}


	struct GpuTextureUploading
	{
		uint32_t firstSubresource;
		uint32_t numSubresources;
		D3D12_SUBRESOURCE_DATA* sources;
	};

	GpuTextureRegistry::GpuTextureRegistry(ID3D12Device5* device)
		: m_device(device)
	{
	}

	uint32_t GpuTextureRegistry::Add(std::string name, GpuUploader& uploader)
	{
		// prevent registering the same name
		if (auto it = m_nameToIndex.find(name); it != m_nameToIndex.end())
		{
			return it->second;
		}

		uint32_t textureIndex;
		if (!m_freeIndices.empty())
		{
			textureIndex = m_freeIndices.back();
			m_freeIndices.pop_back();
		}
		else
		{
			textureIndex = static_cast<uint32_t>(m_resources.size());
			m_resources.push_back(nullptr);
		}

		//DirectX::ScratchImage src_image = ;


		m_nameToIndex.emplace(std::move(name), textureIndex);
		return textureIndex;
	}

}