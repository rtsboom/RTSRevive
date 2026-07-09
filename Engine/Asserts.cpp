#include "pch.h"
#include "Asserts.h"
#include <cstdint>
#include <source_location>
#include <string_view>
#include <intrin.h>
#include <cstdlib>

namespace rr
{
	void AssertFailure(std::string_view expr, std::string_view msg, std::source_location loc)
	{
		if (msg.empty())
		{
			LogOutput(LogLevel::Fatal,
				"Assertion Failure: {}, in file: {}, line: {}",
				expr, loc.file_name(), loc.line());
		}
		else
		{
			LogOutput(LogLevel::Fatal,
				"Assertion Failure: {}, message: {}, in file: {}, line: {}",
				expr, msg, loc.file_name(), loc.line());
		}

		__debugbreak();
		std::abort();
	}

	void CheckFailure(std::string_view expr, std::string_view msg, std::source_location loc)
	{
		if (msg.empty())
		{
			LogOutput(LogLevel::Fatal,
				"Check Failure: {}, in file: {}, line: {}",
				expr, loc.file_name(), loc.line());
		}
		else
		{
			LogOutput(LogLevel::Fatal,
				"Check Failure: {}, message: {}, in file: {}, line: {}",
				expr, msg, loc.file_name(), loc.line());
		}

		__debugbreak();
		std::abort();
	}

	void CheckFailure(std::string_view expr, uint32_t code, std::string_view msg, std::source_location loc)
	{
		if (msg.empty())
		{
			LogOutput(LogLevel::Fatal,
				"Check Failure: {}, code: {}, in file: {}, line: {}",
				expr, code, loc.file_name(), loc.line());
		}
		else
		{
			LogOutput(LogLevel::Fatal,
				"Check Failure: {}, code: {}, message: {}, in file: {}, line: {}",
				expr, code, msg, loc.file_name(), loc.line());
		}
		__debugbreak();
		std::abort();
	}
}
