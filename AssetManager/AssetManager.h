#pragma once

#include "Model.h"
#include "Texture.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
namespace rr
{
	struct IGpuRegistry
	{
	};

	struct IGpuUploader
	{
	};


	using AssetPathCache = std::unordered_map<std::string, uint32_t>;

	struct AssetLevelMark
	{
		uint16_t m_model_count;
		uint16_t m_material_count;
		uint16_t m_texture_count;
		uint16_t m_primitive_count;

		uint16_t m_position_count;
		uint16_t m_skinning_count;
		uint32_t m_vertex_byte_offset;
		uint32_t m_index_byte_offset;
	};

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
		uint32_t LoadModel(std::string_view path);
		Model CreateModel(std::string_view path);



	private:
		std::vector<Texture> m_textures;
		std::vector<Model>   m_models;
		std::vector<Primitive> m_primitives;

		IGpuRegistry* m_gpu_registry;
		IGpuUploader* m_gpu_uploader;
	};
}

