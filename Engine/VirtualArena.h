#pragma once
#include <cstddef>
#include <type_traits>
#include <memory>
#include <utility>
namespace rr
{
	class VirtualArena
	{
	public:
		VirtualArena() = default;
		explicit VirtualArena(size_t max_size);

		~VirtualArena();

		VirtualArena(VirtualArena const&) = delete;
		VirtualArena& operator=(VirtualArena const&) = delete;
		VirtualArena(VirtualArena&& other) noexcept;
		VirtualArena& operator=(VirtualArena&& other) noexcept;


	public:
		void Clear() noexcept;
		void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

		template<typename T>
		T* Allocate()
		{
			static_assert(std::is_trivially_destructible_v<T>);

			return static_cast<T*>(Allocate(sizeof(T), alignof(T)));
		}

		template<typename T>
		T* AllocateArray(size_t count)
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

	private:
		void Swap(VirtualArena& other) noexcept;

	private:
		std::byte* data_{ nullptr };
		size_t used_{ 0 };
		size_t committed_{ 0 };
		size_t reserved_{ 0 };
		size_t page_size_{ 0 };
	};
}

