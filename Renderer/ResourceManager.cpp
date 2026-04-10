#include "pch.h"
#include "ResourceManager.h"
#include "GltfLoader.h"
#include "DxUtils.h"
namespace rrv
{
	ResourceManager::ResourceManager(ID3D12Device5* device, GpuUploader* uploader)
	{
		m_device = device;
		m_uploader = uploader;
	}

	uint32_t ResourceManager::AddModelFromGLTF(std::string fileName)
	{
		GltfLoader::Context ctx = GltfLoader::Load(fileName);

		constexpr Geometry::BufferSlot slotsNeedZFlip[] =
		{
			Geometry::BufferSlot::POSITION,
			Geometry::BufferSlot::NORMAL,
		};

		for (const auto& geometry : ctx.model.geometries)
		{
			// z-flip 
			for (const auto slot : slotsNeedZFlip)
			{
				const Geometry::BufferView& view = geometry.views[u64(slot)];
				if (view.stride == 0) continue;

				for (size_t i = 0; i < geometry.vertexCount; ++i)
				{
					uint8_t* data = ctx.geometry_data.data() + view.offset + i * view.stride + sizeof(float) * 2;
					float* z = reinterpret_cast<float*>(data);
					*z *= -1;
				}
			}

			// convert index winding
			const Geometry::BufferView& view = geometry.views[u64(Geometry::BufferSlot::INDEX)];
			if (view.stride == 2)
			{
				for (size_t i = 0; i < geometry.indexCount; i += 3)
				{
					uint8_t* data = ctx.geometry_data.data() + view.offset + i * view.stride;
					uint16_t* indices = reinterpret_cast<uint16_t*>(data);
					std::swap(indices[1], indices[2]);
				}
			}
			else if (view.stride == 4)
			{
				for (size_t i = 0; i < geometry.indexCount; i += 3)
				{
					uint8_t* data = ctx.geometry_data.data() + view.offset + i * view.stride;
					uint32_t* indices = reinterpret_cast<uint32_t*>(data);
					std::swap(indices[1], indices[2]);
				}
			}
		}

		GpuBuffer gpuBuffer(m_device, ctx.geometry_data.size());
		m_gpuBuffers.push_back(std::move(gpuBuffer));
		m_uploader->EnqueueUploadBuffer(m_gpuBuffers.back().Get(), 0, std::move(ctx.geometry_data));

		ModelBinding modelBinding = {};
		modelBinding.textureSlotStart = static_cast<uint32_t>(m_textures.size());
		modelBinding.textureSlotCount = static_cast<uint32_t>(ctx.scratch_images.size());
		m_model_bindings.push_back(modelBinding);

		m_textures.reserve(ctx.scratch_images.size());
		for (auto& scratch : ctx.scratch_images)
		{
			ComPtr<ID3D12Resource> texture;
			THROW_IF_FAILED(DirectX::CreateTexture(m_device, scratch.GetMetadata(), &texture));
			m_textures.push_back(std::move(texture));
			m_texture_metas.push_back(scratch.GetMetadata());

			m_uploader->EnqueueUploadTexture(m_textures.back().Get(), std::move(scratch));
		}


		m_models.push_back(std::move(ctx.model));
		return u32(m_models.size() - 1);
	}
}