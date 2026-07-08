#include "pch.h"
#include "FileBatch.h"
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>

namespace rr
{
	bool FileBatch::AddFile(std::filesystem::path file_path)
	{
		std::string const path_str = file_path.string();
		std::ifstream file(file_path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			return false;
		}

		std::streampos const end = file.tellg();
		if (end == std::streamsize(-1))
		{
			return false;
		}

		size_t const blob_size = static_cast<size_t>(end);
		file.seekg(0, std::ios::beg);
		if (!file)
		{
			return false;
		}

		constexpr size_t blob_alignment = 16;

		size_t const path_offset = sizeof(FileBlob);
		size_t const path_size = path_str.size() + 1; // include null terminator
		size_t const blob_offset = AlignUp(path_offset + path_size, blob_alignment);
		size_t const total_size = blob_offset + blob_size;

		auto const marker = arena_.UsedSize();
		void* const ptr = arena_.Allocate(total_size, blob_alignment);
		if (!ptr)
		{
			return false;
		}

		FileBlob* file_blob = new (ptr) FileBlob{};

		char* const base = static_cast<char*>(ptr);
		char* const path_begin = base + path_offset;
		std::memcpy(path_begin, path_str.c_str(), path_size);
		file_blob->path = { path_begin, path_size - 1 };

		// read file
		char* const blob_begin = base + blob_offset;
		file.read(blob_begin, blob_size);
		if (!file) // check if read was successful
		{
			arena_.Rollback(marker);
			return false;
		}

		file_blob->bytes = { reinterpret_cast<std::byte*>(blob_begin), blob_size };


		if (last_)
		{
			last_->next = file_blob;
		}
		else
		{
			first_ = file_blob;
		}

		last_ = file_blob;
		return true;
	}
}
