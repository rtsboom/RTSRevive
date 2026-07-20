#pragma once
#include <Engine/StableArena.h>
#include <cstddef>
#include <span>
#include <filesystem>

namespace rr
{
	std::span<std::byte const> ReadFileToBlob(std::filesystem::path file_path, StableArena& arena);
}