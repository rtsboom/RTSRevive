#pragma once

namespace rrv
{
	constexpr int FRAME_COUNT = 2;
}

namespace rrv::RenderConfig
{
	constexpr int SRV_HEAP_SIZE = 4096;
	constexpr int RTV_HEAP_SIZE = 8;
	constexpr int DSV_HEAP_SIZE = 4;

	namespace ShaderRegisterSpace
	{
		enum
		{
			// CBV spaces
			RootCBufferPerFrame,
			RootCBufferPerPass,
			RootCBufferPerDraw,
			RootCBufferCount,

			// Bindless SRV spaces
			Texture2D = RootCBufferCount,
			Texture3D,
			InstanceData,

		};
	}
	constexpr int SRV_TEXTURE2D_START		= 0;
	constexpr int SRV_TEXTURE2D_COUNT		= 64;
	constexpr int SRV_TEXTURE3D_START		= 64;
	constexpr int SRV_TEXTURE3D_COUNT		= 8;
	constexpr int SRV_INSTANCE_DATA_START	= 72;
	constexpr int SRV_INSTANCE_DATA_COUNT	= 1;
	
	constexpr int SRV_MAX_COUNT				= 4096;
	constexpr int CBV_MAX_COUNT				= 3;
}