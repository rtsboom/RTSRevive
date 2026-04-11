#pragma once
#include <Common/Handle.h>

namespace rrv
{
	struct GpuTexture
	{
		// ComPtr<ID3D12Resource> resource;
		// DESC;
	};

	class TexturePool
	{
		struct Slot
		{
			GpuTexture texture;
			uint8_t generation = 0;
			bool    occupied = false;
		};

	public:
		TextureHandle Allocate();
		void          Free(TextureHandle h);
		GpuTexture* Get(TextureHandle h);


	private:
		std::vector<Slot>     m_slots;
		std::vector<uint32_t> m_free_slots;

	
	};
}

