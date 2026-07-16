#pragma once
#include <Engine/StableArena.h>
#include <cstddef>
#include <utility>
namespace rr
{
	// thread local scoped arena allocator
	class TempAllocator
	{
		static constexpr size_t kArenaSize = 64 * 1024 * 1024; // 64 MB
		inline static thread_local StableArena tls_arena_{ kArenaSize };
	public:
		TempAllocator() : marker_{ tls_arena_.UsedSize() } {}
		~TempAllocator() { tls_arena_.Rollback(marker_); }

		TempAllocator(TempAllocator const&) = delete;
		TempAllocator& operator=(TempAllocator const&) = delete;
		TempAllocator(TempAllocator&&) = delete;
		TempAllocator& operator=(TempAllocator&&) = delete;

	public:
		void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t))
		{
			return tls_arena_.Allocate(size, alignment);
		}
		
		template<typename T>
		T* New() { return tls_arena_.New<T>(); }

		template<typename T, typename... Args>
		T* New(Args&&... args) { return tls_arena_.New<T>(std::forward<Args>(args)...); }

		template<typename T>
		T* NewArray(size_t count) { return tls_arena_.NewArray<T>(count); }

	private:
		size_t marker_;
	};
}