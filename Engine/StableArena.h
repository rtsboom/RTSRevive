#pragma once
#include <cstddef>
#include <type_traits>
#include <memory>
#include <utility>
namespace rr
{
	class StableArena
	{
	public:
		StableArena() = default;
		StableArena(StableArena const&) = delete;
		StableArena& operator=(StableArena const&) = delete;
		StableArena(StableArena&& other) noexcept
			: StableArena{}
		{
			Swap(other);
		}
		StableArena& operator=(StableArena&& other) noexcept
		{
			StableArena intermidiate{ std::move(other) };
			Swap(intermidiate);
			return *this;
		}

		explicit StableArena(size_t max_size);
		~StableArena();


	public:
		size_t ReservedSize() const noexcept { return reserved_bytes_; }
		size_t CommittedSize() const noexcept { return committed_bytes_; }
		size_t UsedSize() const noexcept { return used_bytes_; }
		void Rollback(size_t marker);
		void Rewind() noexcept { Rollback(0); }
		void Decommit();
		void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

	public:
		// Separate overload without Args... to avoid IntelliSense false errors.
		template<typename T>
		T* New()
		{
			static_assert(std::is_default_constructible_v<T>,
				"T is not default constructible.");

			T* ptr = static_cast<T*>(Allocate(sizeof(T), alignof(T)));
			if (ptr)
			{
				std::construct_at(ptr);
			}

			return ptr;
		}

		template<typename T, typename ...Args>
		T* New(Args&& ...args)
		{
			static_assert(std::is_constructible_v<T, Args...>,
				"T is not constructible from the provided arguments.");

			T* ptr = static_cast<T*>(Allocate(sizeof(T), alignof(T)));
			if (ptr)
			{
				std::construct_at(ptr, std::forward<Args>(args)...);
			}

			return ptr;
		}

		template<typename T>
		T* NewArray(size_t count)
		{
			static_assert(std::is_default_constructible_v<T>);

			T* ptr = static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
			if (ptr)
			{
				for (size_t i{}; i < count; ++i)
				{
					std::construct_at(&ptr[i]);
				}
			}
			return ptr;
		}

	public:
		void Swap(StableArena& other) noexcept
		{
			std::swap(data_, other.data_);
			std::swap(used_bytes_, other.used_bytes_);
			std::swap(committed_bytes_, other.committed_bytes_);
			std::swap(reserved_bytes_, other.reserved_bytes_);
		}

	private:
		std::byte* data_{ nullptr };
		size_t used_bytes_{ 0 };
		size_t committed_bytes_{ 0 };
		size_t reserved_bytes_{ 0 };
	};
}

