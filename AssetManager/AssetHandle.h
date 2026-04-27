#pragma once
#include <cstdint>

namespace rr
{
	template<typename Tag>
	struct AssetHandle
	{
		uint32_t m_index = UINT32_MAX;
		uint32_t m_generation = 0;

		bool operator==(AssetHandle const& other) const = default;
		bool operator!=(AssetHandle const& other) const = default;
	};

	struct ModelTag {};
	struct TextureTag {};

	using ModelHandle = AssetHandle<ModelTag>;
	using TextureHandle = AssetHandle<TextureTag>;
}