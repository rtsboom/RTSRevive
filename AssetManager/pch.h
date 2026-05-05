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