#pragma once
#include <Shared/RR_Framework.h>

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

#pragma warning(push, 0)
#include <dxgidebug.h>
#pragma warning(pop)

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


#ifdef _DEBUG
#define RR_D3D_CHECK(hr) \
    do { if (FAILED(hr)) { __debugbreak(); } } while (false)
#else
#define RR_D3D_CHECK(hr) \
    do { if (FAILED(hr)) { std::abort(); } } while (false)
#endif

#ifdef _DEBUG
#define  RR_CHECK(condition) \
	do { if (!(condition)) { __debugbreak(); } } while (false)
#else
#define  RR_CHECK(condition) \
	do { if (!(condition)) { std::abort(); } } while (false)
#endif