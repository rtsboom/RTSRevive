#pragma once
#include <Windows.h>
#include <cstdio>
#include <stdexcept>

inline void ThrowIfFailed(HRESULT hr, const char* msg = "")
{

	if (FAILED(hr))
	{
		char buffer[256];
		sprintf_s(buffer, "HRESULT failed: %s (0x%08X)", msg, hr);
		throw std::runtime_error(buffer);
	}
}

#define THROW_IF_FAILED(x) ThrowIfFailed((x), #x)