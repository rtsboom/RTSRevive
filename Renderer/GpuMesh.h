#pragma once
#include "GltfLoader.h"
#include <d3d12.h>

namespace rrv
{
	class GpuMesh
	{
	public:
		GpuMesh() = default;
		GpuMesh(const MeshData& meshData);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
	};
}

