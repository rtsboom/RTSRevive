#pragma once

#include <cstdint>

namespace rrv
{
	template<typename T> constexpr size_t   sz(T v) noexcept { return static_cast<size_t>(v); }
	template<typename T> constexpr uint64_t u64(T v) noexcept { return static_cast<uint64_t>(v); }
	template<typename T> constexpr uint32_t u32(T v) noexcept { return static_cast<uint32_t>(v); }
	template<typename T> constexpr uint16_t u16(T v) noexcept { return static_cast<uint16_t>(v); }
	template<typename T> constexpr uint8_t  u8(T v) noexcept { return static_cast<uint8_t>(v); }
	template<typename T> constexpr int64_t  i64(T v) noexcept { return static_cast<int64_t>(v); }
	template<typename T> constexpr int32_t  i32(T v) noexcept { return static_cast<int32_t>(v); }
	template<typename T> constexpr double   f64(T v) noexcept { return static_cast<double>(v); }
	template<typename T> constexpr float    f32(T v) noexcept { return static_cast<float>(v); }
}