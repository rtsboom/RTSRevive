#pragma once
#include <memory>
#include <cassert>
#include <type_traits>
#include "VirtualMemory.h"
namespace rr
{
	// Utilize virtual memory

	template<typename T>
	class DynamicArray
	{
		static_assert(std::is_default_constructible_v<T>, "DynamicArray requires default constructible type");
	public:
		DynamicArray(size_t capacity)
		{
			size_t const total_bytes = sizeof(T) * capacity;
			first_ = static_cast<T*>(VirtualMemory::Reserve(total_bytes));
			last_ = first_;
			last_reserved_ = first_ + capacity;
		}
		~DynamicArray()
		{
			if (!first_)
				return;

			for (T* e{ first_ }; e != last_; ++e)
			{
				std::destroy_at(e);
			}

			VirtualMemory::Release(first_);
		}
		void Resize(size_t size)
		{
			assert(size <= Capacity());

			T* const new_last = first_ + size;

			if (last_ < new_last)
			{
				size_t const diff = new_last - last_;
				size_t const diff_bytes = sizeof(T) * diff;
				VirtualMemory::Commit(last_, diff_bytes);

				for (T* e{ last_ }; e != new_last; ++e)
				{
					std::construct_at(e);
				}
			}
			else if (last_ > new_last)
			{
				for (T* e{ new_last }; e != last_; ++e)
				{
					std::destroy_at(e);
				}

				size_t const diff = last_ - new_last;
				size_t const diff_bytes = sizeof(T) * diff;

				VirtualMemory::Decommit(new_last, diff_bytes);
			}

			last_ = new_last;
		}

		T& operator[](size_t i)
		{
			assert(first_);
			assert(i < Size());
			return first_[i];
		}


		size_t Size() const noexcept { return last_ - first_; }
		size_t Capacity() const noexcept { return last_reserved_ - first_; }

		T* Data() const noexcept { return first_; }

	private:
		T* first_{ nullptr };
		T* last_{ nullptr };
		T* last_reserved_{ nullptr };
	};

}