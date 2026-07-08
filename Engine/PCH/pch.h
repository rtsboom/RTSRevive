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

#include <Engine/Asserts.h>
#define RR_D3D_CHECK(hr) RR_CHECK(SUCCEEDED(hr))
#define RR_CHECK_WIN32(expr) RR_CHECK_CODE_MSG((expr), GetLastError(), "Win32 API call failed")
#define RR_CHECK_WIN32_MSG(expr, msg) RR_CHECK_CODE_MSG((expr), GetLastError(), msg)

// MathUtils
// TODO: Move this to a more appropriate location.
inline size_t IsPowerOfTwo(size_t value)
{
	return value != 0 && (value & (value - 1)) == 0;
}

inline size_t AlignUp(size_t value, size_t alignment)
{
	RR_ASSERT_MSG(IsPowerOfTwo(alignment), "alignment must be a power of two");

	return (value + alignment - 1) & ~(alignment - 1);
}

