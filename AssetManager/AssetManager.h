#pragma once
#include "AssetHandle.h"
#include "AssetPool.h"
#include "Model.h"
#include "Texture.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
namespace rr
{
	class IGpuRegistry
	{

	};

	class IGpuUploader
	{

	};


	using AssetPathCache = std::unordered_map<std::string, uint32_t>;

	class AssetManager
	{
	public:
		AssetManager() = default;
		~AssetManager() noexcept = default;
		AssetManager(AssetManager&&) noexcept = default;
		AssetManager& operator=(AssetManager&&) noexcept = default;

		// Non Copyable
		AssetManager(AssetManager const&) = delete;
		AssetManager& operator=(AssetManager const&) = delete;

	public:
		ModelHandle LoadModel(std::string_view path);

		void RemoveModel(ModelHandle handle) { m_model_pool.Remove(handle); }
		void RemoveTexture(TextureHandle handle) { m_texture_pool.Remove(handle); }
		
		Model& GetModel(ModelHandle handle) { return m_model_pool.Get(handle); }
		Model const& GetModel(ModelHandle handle) const { return m_model_pool.Get(handle); }

		Texture& GetTexture(TextureHandle handle) { return m_texture_pool.Get(handle); }
		Texture const& GetTexture(TextureHandle handle) const { return m_texture_pool.Get(handle); }

	private:
		AssetPool<Model, ModelHandle> m_model_pool;
		AssetPool<Texture, TextureHandle> m_texture_pool;

		IGpuRegistry* m_gpu_registry;
		IGpuUploader* m_gpu_uploader;
	};
}

