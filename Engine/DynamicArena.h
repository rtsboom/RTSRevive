#pragma once
#include <cstddef>
#include <type_traits>
#include <memory>
#include <utility>
namespace rr
{
	class DynamicArena
	{
	public:
		DynamicArena() = default;
		explicit DynamicArena(size_t max_size);
		DynamicArena(DynamicArena const&) = delete;
		DynamicArena& operator=(DynamicArena const&) = delete;
		DynamicArena(DynamicArena&& other) noexcept;
		DynamicArena& operator=(DynamicArena&& other) noexcept;
		~DynamicArena();


	public:
		size_t MaxSize() const noexcept { return reserved_bytes_; }
		size_t GetMarker() const noexcept { return used_bytes; }
		void Rollback(size_t marker);
		void Clear() noexcept { used_bytes = 0; }
		void Reset();
		void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

		template<typename T>
		T* Allocate(size_t count = 1)
		{
			static_assert(std::is_trivially_destructible_v<T>);

			return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
		}

		template<typename T, typename ...Args>
		T* New(Args&& ...args)
		{
			T* ptr = Allocate<T>();
			if (ptr)
			{
				std::construct_at(ptr, std::forward<Args>(args)...);
			}

			return ptr;
		}

		template<typename T, typename ...Args>
		T* NewArray(size_t count, Args&& ...args)
		{
			T* ptr = Allocate<T>(count);
			if (ptr)
			{
				for (size_t i{}; i < count; ++i)
				{
					std::construct_at(&ptr[i], std::forward<Args>(args)...);
				}
			}
			return ptr;
		}

	public:
		void Swap(DynamicArena& other) noexcept;

	private:
		std::byte* data_{ nullptr };
		size_t used_bytes{ 0 };
		size_t committed_bytes_{ 0 };
		size_t reserved_bytes_{ 0 };
	};
}

