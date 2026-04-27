#pragma once

#include "AssetHandle.h"
#include <CastUtils.h>
#include <vector>
#include <cassert>

namespace rr
{
	template<typename T, typename THandle>
	class AssetPool
	{
	private:
		struct Slot
		{
			T	     m_item;
			uint32_t m_generation = 0;
			bool     m_occupied = false;

		};

		std::vector<Slot> m_slots;
		std::vector<uint32_t> m_free_list;

	public:
		bool IsValid(THandle handle) const
		{
			if (m_slots.size() <= handle.m_index) return false;

			Slot const& slot = m_slots[handle.m_index];
			return slot.m_occupied && slot.m_generation == handle.generation;
		}

		THandle Add(T item)
		{
			uint32_t index;
			if (!m_free_list.empty())
			{
				index = m_free_list.back();
				m_free_list.pop_back();
			}
			else
			{
				index = u32(m_slots.size());
				m_slots.push_back({});
			}

			Slot& slot = m_slots[index];
			slot.m_item = std::move(item);
			slot.m_occupied = true;

			THandle handle = {
				.m_index = index,
				.m_generation = slot.m_generation
			};
			return handle;
		}

		void Remove(THandle handle)
		{
			assert(IsValid(handle));
			Slot& slot = m_slots[handle.m_index];
			slot.m_item = {};
			slot.m_occupied = false;
			slot.m_generation += 1;
			m_free_list.push_back(handle.m_index);
		}

		T& Get(THandle handle)
		{
			assert(IsValid(handle));
			return m_slots[handle.m_index].m_item;
		}

		T const& Get(THandle handle) const
		{
			assert(IsValid(handle));
			return m_slots[handle.m_index].m_item;
		}
	};
}