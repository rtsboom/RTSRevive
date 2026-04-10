#include "pch.h"
#include "Pipeline.h"
#include "DxUtils.h"
#include "PathUtils.h"

#include "RenderConfig.h"

#include <Windows.h>
#include <cstddef>
#include <cstdlib> 
#include <filesystem>
#include <fstream>

using namespace Microsoft::WRL;

static std::vector<std::byte> ReadFileToBlob(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	assert(file.is_open() && "failed to open file");

	if (!file.is_open())
		return {};

	const size_t fileSize = file.tellg();
	assert(fileSize > 0 && "file is empty");

	file.seekg(0, std::ios::beg);
	std::vector<std::byte> buffer(fileSize);
	file.read((char*)buffer.data(), fileSize);

	return buffer;
}

void Pipeline::Init(ID3D12Device5* device)
{
	m_globalRootSign = CreateGlobalRootSignature(device);

	auto executableDir = GetExecutableDirectoryPath();

	auto staticMeshVSBlob = ReadFileToBlob(executableDir / L"StaticMeshVS.cso");
	auto staticMeshPSBlob = ReadFileToBlob(executableDir / L"StaticMeshPS.cso");

	//D3DReadFileToBlob
	// PSO
	struct
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;

		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat = DXGI_FORMAT_D32_FLOAT;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats = D3D12_RT_FORMAT_ARRAY({ DXGI_FORMAT_R8G8B8A8_UNORM }, 1);
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	} psoStream{};

	psoStream.pRootSignature = m_globalRootSign.Get();
	psoStream.VS = { staticMeshVSBlob.data(), staticMeshVSBlob.size() };
	psoStream.PS = { staticMeshPSBlob.data(), staticMeshPSBlob.size() };

	// input layout
	// slot 0 : position
	// slot 1 : normal, texcoord
	D3D12_INPUT_ELEMENT_DESC elems[] =
	{
		"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
		"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
		"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
	};

	psoStream.InputLayout = { elems, _countof(elems) };

	D3D12_PIPELINE_STATE_STREAM_DESC psoStreamDesc = {};
	psoStreamDesc.SizeInBytes = sizeof(psoStream);
	psoStreamDesc.pPipelineStateSubobjectStream = &psoStream;
	THROW_IF_FAILED(device->CreatePipelineState(&psoStreamDesc, IID_PPV_ARGS(&m_staticMeshPso)));


}

ComPtr<ID3D12RootSignature> Pipeline::CreateGlobalRootSignature(ID3D12Device* device)
{
	CD3DX12_DESCRIPTOR_RANGE1 ranges[3] = {};
	{
		int rangeIndex = 0;
		// instance data
		ranges[rangeIndex++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, rrv::RenderConfig::SRV_TEXTURE2D_COUNT,
			0, rrv::RenderConfig::ShaderRegisterSpace::Texture2D,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		// textures
		ranges[rangeIndex++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, rrv::RenderConfig::SRV_TEXTURE3D_COUNT,
			0, rrv::RenderConfig::ShaderRegisterSpace::Texture3D,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		ranges[rangeIndex++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, rrv::RenderConfig::SRV_INSTANCE_DATA_COUNT,
			0, rrv::RenderConfig::ShaderRegisterSpace::InstanceData,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	}

	const UINT numParams = rrv::RenderConfig::ShaderRegisterSpace::RootCBufferCount + 1/* srv descriptor table */;
	CD3DX12_ROOT_PARAMETER1 params[numParams] = {};
	{
		int paramIndex = 0;
		params[paramIndex++].InitAsConstantBufferView(0, rrv::RenderConfig::ShaderRegisterSpace::RootCBufferPerFrame);
		params[paramIndex++].InitAsConstantBufferView(0, rrv::RenderConfig::ShaderRegisterSpace::RootCBufferPerPass);
		params[paramIndex++].InitAsConstantBufferView(0, rrv::RenderConfig::ShaderRegisterSpace::RootCBufferPerDraw);
		params[paramIndex++].InitAsDescriptorTable(_countof(ranges), ranges);
	}

	CD3DX12_STATIC_SAMPLER_DESC sampler(0);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignDesc = {};
	rootSignDesc.Init_1_1(numParams, params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSignBlob;
	ComPtr<ID3DBlob> errorBlob;

	THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&rootSignDesc, &rootSignBlob, &errorBlob));

	ComPtr<ID3D12RootSignature> rootSign;
	THROW_IF_FAILED(device->CreateRootSignature(0,
		rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSign)));

	return rootSign;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Pipeline::CreateStaticMeshPso(ID3D12Device* device)
{
	return Microsoft::WRL::ComPtr<ID3D12PipelineState>();
}
