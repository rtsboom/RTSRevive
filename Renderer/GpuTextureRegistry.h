#pragma once
#include <unordered_map>
#include "GpuUploader.h"
namespace rrv
{
	using Microsoft::WRL::ComPtr;

	template <typename TTexture>
	struct TextureHandle
	{
		uint32_t index;
	};

	struct Texture2D
	{
	};


	struct Texture2DArray
	{
	};
	struct Texture3D
	{
	};



	class GpuTextureRegistry
	{
	public:
		~GpuTextureRegistry() = default;
		GpuTextureRegistry(const GpuTextureRegistry&) = delete;
		GpuTextureRegistry& operator=(const GpuTextureRegistry&) = delete;
		GpuTextureRegistry(GpuTextureRegistry&&) = delete;
		GpuTextureRegistry& operator=(GpuTextureRegistry&&) = delete;
	public:
		GpuTextureRegistry(ID3D12Device5* device);

		uint32_t Add(std::string name, GpuUploader& uploader);
		void Remove(uint32_t textureIndex);
		ID3D12Resource* GetResource(uint32_t textureIndex) const;

	private:
		ID3D12Device5* m_device = nullptr;
		std::vector<ComPtr<ID3D12Resource>> m_resources;

		std::unordered_map<std::string, uint32_t> m_nameToIndex;
		std::vector<uint32_t> m_freeIndices;
	};
}
