#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
namespace rr
{
	using ModelHandle = uint32_t;    // Blender Scene
	using MeshHandle = uint32_t;     // Blender Object
	using MaterialHandle = uint32_t; // Blender Material

	using StagingBuffer = std::vector<uint8_t>;
	using StagingTexture = DirectX::ScratchImage;
	struct ByteSpan { uint32_t offset, length, stride; };

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
		ModelHandle LoadModelFromGLTF2(std::string_view path);
		uint32_t LoadModelFromGLTF(std::string_view path);

	private:
		AssetPathCache m_image_path_cache;

		uint32_t m_model_count = 0;
		uint32_t m_image_count = 0;
	};
}

