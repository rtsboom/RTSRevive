#pragma once
#include <RR_Framework.h>

#pragma warning(push, 0)
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#pragma warning(pop)

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#define RR_D3D12_DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

namespace rr
{
	// WRL
	using Microsoft::WRL::ComPtr;

	// DirectXMath
	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT4;
	using DirectX::XMFLOAT4X4;
	using DirectX::XMMATRIX;
	using DirectX::XMVECTOR;

	// DirectXTex
	using DirectX::ScratchImage;
	using DirectX::TexMetadata;
}

#include <stdexcept>

inline void ThrowIfFailed(HRESULT hr, const char* msg = "")
{

	if (FAILED(hr))
	{
		char buffer[256];
		sprintf_s(buffer, "HRESULT failed: %s (0x%08X)", msg, hr);
		throw std::runtime_error(buffer);
	}
}

#define THROW_IF_FAILED(x) ThrowIfFailed((x), #x)