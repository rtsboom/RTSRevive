#pragma once
// Precompiled header for the Renderer project

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#pragma warning(push, 0)
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#pragma warning(pop)

namespace rrv
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


#include <cstdint>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <utility>
#include <stdexcept>

#include <Common/MathUtils.h>
#include <Common/CastUtils.h>