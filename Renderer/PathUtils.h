#pragma once
#include <Windows.h>

#include <filesystem>
#include <string>

inline const std::filesystem::path& GetExecutableDirectoryPath()
{
	static const std::filesystem::path dir = []() {
		wchar_t buffer[MAX_PATH];
		GetModuleFileNameW(NULL, buffer, MAX_PATH);
		std::filesystem::path executablePath(buffer);
		return executablePath.parent_path();
		}();

	return dir;
}