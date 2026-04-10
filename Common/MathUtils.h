#pragma once
#include <numbers>
#include <concepts>
#include <cstdint>

namespace rrv
{
	constexpr float PI = std::numbers::pi_v<float>;
	struct Vec2
	{
		float x, y;
	};

	struct Vec3
	{
		float x, y, z;
	};

	struct Vec4
	{
		float x, y, z, w;
	};

	template <std::unsigned_integral T, std::unsigned_integral U>
	constexpr T AlignUp(T value, U alignment)
	{
		// check power of two
		assert((alignment & (alignment - 1)) == 0);

		return (value + static_cast<T>(alignment) - 1) & ~(static_cast<T>(alignment) - 1);
	}
}