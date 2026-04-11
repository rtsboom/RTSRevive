#pragma once
#include <cstdint>
namespace rrv
{
	template<typename Tag>
	struct Handle
	{
		Handle() = default;
		Handle(uint32_t index, uint32_t generation) : value((index & 0x00FF'FFFF) | ((generation & 0xFF) << 24)) {}

		uint32_t Index() const { return value & 0x00FF'FFFF; }
		uint32_t Generation() const { return (value >> 24) & 0xFF; }

		bool IsValid() const { return value != 0x00FF'FFFF; }
		static Handle Invalid() { return {}; }

		bool operator==(const Handle&) const = default;

	private:
		uint32_t value = 0xFFFF'FFFF;
	};

	struct TextureTag {};
	struct GeometryTag {};

	using TextureHandle = Handle<TextureTag>;
	using GeometryHandle = Handle<GeometryTag>;

}