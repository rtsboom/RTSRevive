#pragma once
#include <cassert>
#include <concepts>

namespace rr
{
	template <std::unsigned_integral T>
	constexpr T AlignUp(T value, T align)
	{
		assert(align > 0);
		assert((align & (align - 1)) == 0);
		return (value + (align - 1)) & ~(align - 1);
	}

	template <auto Align, std::unsigned_integral T>
	constexpr T AlignUp(T value)
	{
		static_assert(Align > 0);
		static_assert((Align & (Align - 1)) == 0);
		return (value + (Align - 1)) & ~(Align - 1);
	}
}