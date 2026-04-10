#pragma once
namespace rrv
{
	using Microsoft::WRL::ComPtr;
	struct GpuTexture
	{
		ComPtr<ID3D12Resource> resource;
		D3D12_RESOURCE_DESC desc;
	};

}
