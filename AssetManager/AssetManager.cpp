#include "pch.h"
#include "AssetManager.h"
#include "GLTFLoader.h"
#include <unordered_map>
#include <string>
namespace rrv
{
	using ImagePathCache = std::unordered_map<std::string, uint32_t>;
	struct AssetManagerImpl
	{
		Renderer* m_renderer = nullptr;
		uint32_t m_model_count = 0;
		uint32_t m_image_count = 0;
		ImagePathCache m_image_path_cache;

	public:
		uint32_t LoadModelFromGLTF(std::string_view path);
	};

	uint32_t AssetManagerImpl::LoadModelFromGLTF(std::string_view path)
	{
		GLTFLoader::Context ctx = GLTFLoader::Load(path, m_image_path_cache);

		// Matrix Z Flip
		for (size_t i = 0; i < ctx.world_matrices.size(); ++i)
		{
			XMMATRIX m = XMLoadFloat4x4(&ctx.world_matrices[i]);
			m *= XMMatrixScaling(1.f, 1.f, -1.f);
			XMStoreFloat4x4(&ctx.world_matrices[i], m);
		}

		constexpr Asset::Geometry::BufferSlot slotsNeedZFlip[] =
		{
			 Asset::Geometry::BufferSlot::POSITION,
			 Asset::Geometry::BufferSlot::NORMAL,
		};

		for (const auto& geometry : ctx.model.geometries)
		{
			// Vertex Z Flip
			for (const auto slot : slotsNeedZFlip)
			{
				auto const& view = geometry.views[u64(slot)];
				if (view.stride == 0) continue;

				for (size_t i = 0; i < geometry.vertex_count; ++i)
				{
					uint8_t* data = ctx.geometry_data.data() + view.offset + i * view.stride + sizeof(float) * 2;
					float* z = reinterpret_cast<float*>(data);
					*z *= -1;
				}
			}

			// Convert Index Winding
			auto const& view = geometry.views[u64(Asset::Geometry::BufferSlot::INDEX)];
			if (view.stride == 2)
			{
				for (size_t i = 0; i < geometry.index_count; i += 3)
				{
					uint8_t* data = ctx.geometry_data.data() + view.offset + i * view.stride;
					uint16_t* indices = reinterpret_cast<uint16_t*>(data);
					std::swap(indices[1], indices[2]);
				}
			}
			else if (view.stride == 4)
			{
				for (size_t i = 0; i < geometry.index_count; i += 3)
				{
					uint8_t* data = ctx.geometry_data.data() + view.offset + i * view.stride;
					uint32_t* indices = reinterpret_cast<uint32_t*>(data);
					std::swap(indices[1], indices[2]);
				}
			}
		}

		return m_model_count++;
	}

	AssetManager::AssetManager() = default;
	AssetManager::~AssetManager() = default;
	AssetManager::AssetManager(AssetManager&&) noexcept = default;
	AssetManager& AssetManager::operator=(AssetManager&&) noexcept = default;

	AssetManager::AssetManager(Renderer* renderer)
		: m_impl(std::make_unique<AssetManagerImpl>())
	{
		m_impl->m_renderer = renderer;
	}

	uint32_t AssetManager::LoadModelFromGLTF(std::string_view path)
	{
		return m_impl->LoadModelFromGLTF(std::move(path));

	}





}