#pragma once
#include <intrin.h>
#include <cstdint>

#include <source_location>
#include <string_view>
#include <format>
#include <Windows.h>

namespace rr
{
	enum class LogLevel
	{
		Fatal,
		Error,
		Warning,
		Info,
		Debug,
		Count
	};

	constexpr char const* GetLogLevelString(LogLevel level)
	{
		constexpr size_t level_string_count = static_cast<size_t>(LogLevel::Count);
		constexpr char const* level_strings[level_string_count] =
		{
			"FATAL",
			"ERROR",
			"WARN",
			"INFO",
			"DEBUG"
		};
		size_t const level_index = static_cast<size_t>(level);
		if (level_index < level_string_count)
			return level_strings[level_index];

		return "UNKNOWN";
	}

	template<typename... Args>
	inline void LogOutput(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
	{
		char buffer[1024];
		constexpr size_t buffer_size = sizeof(buffer) - 2;
		char* out = buffer;
		char* end = buffer + buffer_size;

		auto r1 = std::format_to_n(out, end - out, "[{}]", GetLogLevelString(level));
		out = r1.out;

		if (out < end)
		{
			auto r2 = std::format_to_n(out, end - out, fmt, std::forward<Args>(args)...);
			out = r2.out;
		}

		*out++ = '\n';
		*out = '\0';
		OutputDebugStringA(buffer);
	}

	void AssertFailure(std::string_view expr, std::string_view msg = {}, std::source_location loc = std::source_location::current());
	void CheckFailure(std::string_view expr, std::string_view msg = {}, std::source_location loc = std::source_location::current());
	void CheckFailure(std::string_view expr, uint32_t code, std::string_view msg = {}, std::source_location loc = std::source_location::current());
}

#define RR_CHECK(expr) do { if (!(expr)) CheckFailure(#expr); } while(false)
#define RR_CHECK_MSG(expr, msg)	do { if (!(expr)) CheckFailure(#expr, msg); } while(false)
#define RR_CHECK_CODE_MSG(expr, code, msg) do { if (!(expr)) CheckFailure(#expr, code, msg); } while(false)

#ifdef NDEBUG
#define RR_ASSERT(expr) ((void)0)
#define RR_ASSERT_MSG(expr, msg) ((void)0)
#else
#define RR_ASSERT(expr) do { if (!(expr)) rr::AssertFailure(#expr); } while (false)
#define RR_ASSERT_MSG(expr, msg) do { if(!(expr)) rr::AssertFailure(#expr, msg);} while(false)
#endif
