#pragma once
#include <cstdint>
#include <memory>
#include <string_view>

#include <Renderer/Renderer.h>
namespace rrv
{
	// Forward Declaration for Pimpl
	struct AssetManagerImpl;

	class AssetManager
	{
	public:
		AssetManager();
		~AssetManager();
		AssetManager(AssetManager&&) noexcept;
		AssetManager& operator=(AssetManager&&) noexcept;
		// Non Copyable
		AssetManager(AssetManager const&) = delete;
		AssetManager& operator=(AssetManager const&) = delete;

		AssetManager(Renderer* renderer);
	public:
		uint32_t LoadModelFromGLTF(std::string_view path);

	private:
		std::unique_ptr<rrv::AssetManagerImpl> m_impl;
	};
}

