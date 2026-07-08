#pragma once
#include "VirtualMemory.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <type_traits>
#include <algorithm>

namespace rr
{
	// Utilize virtual memory

	template<typename T>
	class StableArray
	{
	public:
		StableArray() = default;
		StableArray(StableArray const&) = delete;
		StableArray& operator=(StableArray const&) = delete;
		StableArray(StableArray&& other) noexcept
			: StableArray{}
		{
			Swap(other);
		}
		StableArray& operator=(StableArray&& other) noexcept
		{
			StableArray intermidiate{ std::move(other) };
			Swap(intermidiate);
			return *this;
		}

		explicit StableArray(size_t max_size)
		{
			assert(max_size > 0);

			size_t const max_bytes = sizeof(T) * max_size;
			size_t const max_bytes_aligned =
				VirtualMemory::AlignToAllocationGranularity(max_bytes);

			void* const ptr = VirtualMemory::Reserve(max_bytes_aligned);
			data_ = static_cast<T*>(ptr);
			max_size_ = max_bytes_aligned / sizeof(T);
			reserved_bytes_ = max_bytes_aligned;
		}
		~StableArray()
		{
			if (!data_)
				return;

			for (size_t i{}; i < size_; ++i)
			{
				std::destroy_at(&data_[i]);
			}

			VirtualMemory::Release(data_);
		}

		void Reserve(size_t new_capacity)
		{
			assert(new_capacity <= MaxSize());

			if (capacity_ < new_capacity)
			{
				size_t const required_bytes = sizeof(T) * new_capacity;
				size_t const required_bytes_aligned = VirtualMemory::AlignToPageSize(required_bytes);

				std::byte* const committed_end = ByteData() + committed_bytes_;
				size_t const commit_bytes = required_bytes_aligned - committed_bytes_;
				VirtualMemory::Commit(committed_end, commit_bytes);

				committed_bytes_ = required_bytes_aligned;
				capacity_ = committed_bytes_ / sizeof(T);
			}
		}

		void ShrinkToFit()
		{
			size_t const required_bytes = sizeof(T) * size_;
			size_t const required_bytes_aligned = VirtualMemory::AlignToPageSize(required_bytes);

			if (required_bytes_aligned < committed_bytes_)
			{
				std::byte* const decommit_begin = ByteData() + required_bytes_aligned;
				size_t const decommit_bytes = committed_bytes_ - required_bytes_aligned;
				VirtualMemory::Decommit(decommit_begin, decommit_bytes);

				committed_bytes_ = required_bytes_aligned;
				capacity_ = committed_bytes_ / sizeof(T);
			}
		}

		void Resize(size_t size)
		{
			static_assert(std::is_default_constructible_v<T>);

			assert(size <= MaxSize());

			if (size_ < size)
			{
				Reserve(size);
				for (size_t i{ size_ }; i < size; ++i)
					std::construct_at(&data_[i]);   // default construct
			}
			else if (size < size_)
			{
				for (size_t i{ size }; i < size_; ++i)
					std::destroy_at(&data_[i]);
			}

			size_ = size;
		}

		void Resize(size_t size, T const& value)
		{
			assert(size <= MaxSize());

			if (size_ < size)
			{
				Reserve(size);
				for (size_t i{ size_ }; i < size; ++i)
				{
					std::construct_at(&data_[i], value);
				}
			}
			else if (size < size_)
			{
				for (size_t i{ size }; i < size_; ++i)
				{
					std::destroy_at(&data_[i]);
				}
			}

			size_ = size;
		}

		void Clear()
		{
			Resize(0);
		}

		template<typename ...Args>
		void EmplaceBack(Args&& ...args)
		{
			static_assert(std::is_constructible_v<T, Args...>);
			Reserve(size_ + 1);
			std::construct_at(&data_[size_++], std::forward<Args>(args)...);
		}

		void PushBack(T const& value)
		{
			Reserve(size_ + 1);
			std::construct_at(&data_[size_++], value);
		}

		void PushBack(T&& value)
		{
			Reserve(size_ + 1);
			std::construct_at(&data_[size_++], std::move(value));
		}

		void PopBack()
		{
			assert(0 < size_);
			std::destroy_at(&data_[--size_]);
		}

		T& operator[](size_t i) { return data_[i]; }
		T const& operator[](size_t i) const { return data_[i]; }

		T& Front() { return data_[0]; }
		T& Back() { return data_[size_ - 1]; }
		T const& Front() const { return data_[0]; }
		T const& Back() const { return data_[size_ - 1]; }


		size_t Size() const noexcept { return size_; }
		size_t Capacity() const noexcept { return capacity_; }
		size_t MaxSize() const noexcept { return max_size_; }

		T* Data() noexcept { return data_; }
		T const* Data() const noexcept { return data_; }

	private:
		std::byte* ByteData() noexcept { return reinterpret_cast<std::byte*>(data_); }
		std::byte const* ByteData() const noexcept { return reinterpret_cast<std::byte const*>(data_); }

	public:
		void Swap(StableArray& other) noexcept
		{
			std::swap(data_, other.data_);
			std::swap(size_, other.size_);
			std::swap(capacity_, other.capacity_);
			std::swap(max_size_, other.max_size_);
			std::swap(committed_bytes_, other.committed_bytes_);
			std::swap(reserved_bytes_, other.reserved_bytes_);
		}

	private:
		T* data_{ nullptr };
		size_t size_{ 0 };
		size_t capacity_{ 0 };
		size_t max_size_{ 0 };
		size_t committed_bytes_{ 0 };
		size_t reserved_bytes_{ 0 };
	};
}