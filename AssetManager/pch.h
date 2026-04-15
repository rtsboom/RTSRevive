#pragma once

#include <DirectXMath.h> // Windows SDK
#include <DirectXTex.h>  // DirectXTex Nuget Pakage

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