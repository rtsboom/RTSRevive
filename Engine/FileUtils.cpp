#include "pch.h"
#include "FileUtils.h"
#include <Engine/StableArena.h>
#include <cstddef>
#include <span>
#include <filesystem>
#include <fstream>

std::span<std::byte const> rr::ReadFileToBlob(std::filesystem::path file_path, StableArena& arena)
{
	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
	if (!file) // check if file opened successfully
	{
		return {};
	}

	std::streampos const end = file.tellg();
	if (end == std::streamsize(-1))
	{
		return {};
	}

	size_t const blob_size = static_cast<size_t>(end);
	file.seekg(0, std::ios::beg);
	if (!file)
	{
		return {};
	}

	auto const marker = arena.UsedSize();
	void* const blob = arena.Allocate(blob_size);
	if (!blob)
	{
		return {};
	}

	// read file
	file.read(static_cast<char*>(blob), blob_size);
	if (!file) // check if read was successful
	{
		arena.Rollback(marker);
		return {};
	}

	return { static_cast<std::byte const*>(blob), blob_size };
}
