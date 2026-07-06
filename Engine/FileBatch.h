#pragma once
#include <Engine/DynamicArena.h>
#include <cstddef>
#include <string_view>
#include <filesystem>
#include <span>
namespace rr
{
	struct FileBlob
	{
		std::string_view path;
		std::span<std::byte const> bytes;
		FileBlob* next;
	};

	class FileBatch
	{
	public:
		FileBatch() = default;
		FileBatch(size_t capacity_bytes)
		{
			arena_ = DynamicArena(capacity_bytes);
		}

		bool AddFile(std::filesystem::path file_path);
		void Clear() { arena_.Clear(); first_ = nullptr; last_ = nullptr; }

	private:
		DynamicArena arena_;
		FileBlob* first_{ nullptr };
		FileBlob* last_{ nullptr };
	};
}