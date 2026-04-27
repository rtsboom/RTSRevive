#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
namespace rr
{
	using ModelHandle = uint32_t;   
	using MeshHandle = uint32_t;    
	using MaterialHandle = uint32_t;


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

	private:
		AssetPathCache m_path_cache;

		uint32_t m_model_count = 0;
		uint32_t m_image_count = 0;
	};
}

