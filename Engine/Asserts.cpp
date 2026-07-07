#include "pch.h"
#include <Windows.h>
#include <cstdio>
#include <cstdint>

void rr::ReportFailure(const char* expr, const char* msg, const char* file, int line) noexcept
{
	// TODO: need logging system

	char buffer[1024];
	std::snprintf(
		buffer,
		sizeof(buffer),
		"Assertion failed!\nExpression: %s\nMessage: %s\nFile: %s\nLine: %d\n",
		expr,
		msg ? msg : " ",
		file,
		line
	);

	OutputDebugStringA(buffer);
}


void rr::ReportFailure(const char* expr, const char* msg, uint32_t err, const char* file, int line) noexcept
{
	// TODO: need logging system

	char buffer[1024];
	std::snprintf(
		buffer, 
		sizeof(buffer),
		"Assertion failed!\nExpression: %s\nMessage: %s\nErrorCode: 0x%08X\nFile: %s\nLine: %d\n",
		expr, 
		msg ? msg : " ", 
		err,
		file, 
		line
	);

	OutputDebugStringA(buffer);
}
