#pragma once

#pragma warning(push, 0)
#include <d3dx12.h>
#include <DirectXMath.h> // Windows SDK
#include <DirectXTex.h>  // DirectXTex Nuget Pakage
#pragma warning(pop)
namespace rr
{
	// DirectXMath
	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT4;
	using DirectX::XMFLOAT4X4;
	using DirectX::XMMATRIX;
	using DirectX::XMVECTOR;

	using DirectX::XMMatrixScaling;
	using DirectX::XMMatrixIdentity;
	using DirectX::XMVectorSet;
	using DirectX::XMMatrixRotationQuaternion;
	using DirectX::XMMatrixTranslation;
	using DirectX::XMLoadFloat4x4;

	// DirectXTex
	using DirectX::ScratchImage;
	using DirectX::TexMetadata;
}


#include <CastUtils.h>
#include <MathUtils.h>

#include <cstdint>
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