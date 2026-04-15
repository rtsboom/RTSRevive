#include "pch.h"
#include "AssetManager.h"

namespace rr
{
	ModelHandle AssetManager::LoadModelFromGLTF2(std::string_view path)
	{
		struct GLTFContext;



		return ModelHandle();
	}
	//uint32_t AssetManager::LoadModelFromGLTF(std::string_view path)
	//{
	//	ModelLoader::Context ctx = ModelLoader::LoadFromGLTF(path, m_image_path_cache);

	//	// Matrix Z Flip
	//	for (size_t i = 0; i < ctx.world_matrices.size(); ++i)
	//	{
	//		XMMATRIX m = XMLoadFloat4x4(&ctx.world_matrices[i]);
	//		m *= XMMatrixScaling(1.f, 1.f, -1.f);
	//		XMStoreFloat4x4(&ctx.world_matrices[i], m);
	//	}

	//	constexpr Asset::Geometry::BufferSlot slotsNeedZFlip[] =
	//	{
	//		 Asset::Geometry::BufferSlot::POSITION,
	//		 Asset::Geometry::BufferSlot::NORMAL,
	//	};

	//	for (const auto& geometry : ctx.model.geometries)
	//	{
	//		// Vertex Z Flip
	//		for (const auto slot : slotsNeedZFlip)
	//		{
	//			auto const& view = geometry.views[u64(slot)];
	//			if (view.stride == 0) continue;

	//			for (size_t i = 0; i < geometry.vertex_count; ++i)
	//			{
	//				uint8_t* data = ctx.staging_buffer.data() + view.offset + i * view.stride + sizeof(float) * 2;
	//				float* z = reinterpret_cast<float*>(data);
	//				*z *= -1;
	//			}
	//		}

	//		// Convert Index Winding
	//		auto const& view = geometry.views[u64(Asset::Geometry::BufferSlot::INDEX)];
	//		if (view.stride == 2)
	//		{
	//			for (size_t i = 0; i < geometry.index_count; i += 3)
	//			{
	//				uint8_t* data = ctx.staging_buffer.data() + view.offset + i * view.stride;
	//				uint16_t* indices = reinterpret_cast<uint16_t*>(data);
	//				std::swap(indices[1], indices[2]);
	//			}
	//		}
	//		else if (view.stride == 4)
	//		{
	//			for (size_t i = 0; i < geometry.index_count; i += 3)
	//			{
	//				uint8_t* data = ctx.staging_buffer.data() + view.offset + i * view.stride;
	//				uint32_t* indices = reinterpret_cast<uint32_t*>(data);
	//				std::swap(indices[1], indices[2]);
	//			}
	//		}
	//	}

	//	return m_model_count++;
	//}





}