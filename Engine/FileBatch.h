#pragma once
#include <Engine/StableArena.h>
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
			arena_ = StableArena(capacity_bytes);
		}

		bool AddFile(std::filesystem::path file_path);
		void Clear() { arena_.Rewind(); first_ = nullptr; last_ = nullptr; }

	public:
		template<typename Fn>
		void ForEach(Fn&& fn) const
		{
			static_assert(std::is_invocable_v<Fn&, FileBlob const&>);

			FileBlob* current = first_;
			while (current)
			{
				std::invoke(fn, *current);
				current = current->next;
			}
		}


	public:
		size_t GetArenaMaxSize() const noexcept { return arena_.ReservedSize(); }
		size_t GetUsedSize() const noexcept { return arena_.UsedSize(); }

	private:
		StableArena arena_;
		FileBlob* first_{ nullptr };
		FileBlob* last_{ nullptr };
	};
}