#pragma once
#include "RenderTypes.h"
#include "GpuUploader.h"
#include "GpuBuffer.h"
namespace rrv
{
	using Microsoft::WRL::ComPtr;

	struct ModelBinding
	{
		uint32_t textureSlotStart;
		uint32_t textureSlotCount;
	};


	class ResourceManager
	{
	public:
		ResourceManager() = default;
		ResourceManager(ID3D12Device5* device, GpuUploader* uploader);

	private:
		ID3D12Device5* m_device = nullptr;
		GpuUploader* m_uploader = nullptr;

		std::vector<Model>		  m_models;
		std::vector<ModelBinding> m_model_bindings;

		std::vector<GpuBuffer>				m_gpuBuffers;
		std::vector<ComPtr<ID3D12Resource>> m_textures;
		std::vector<DirectX::TexMetadata>   m_texture_metas;


		// TODO: Need slot reuse mechanism when unloading assets
	};


}