#include "pch.h"
#include "Renderer.h"
#include "GltfLoader.h"
#include "DxUtils.h"

#include "RenderConfig.h"
#include "Pipeline.h"
#include "ConstantBuffer.h"
#include "InstanceBuffer.h"
#include "GpuFence.h"
#include "GpuEvent.h"

#include "GpuUploader.h"

using Microsoft::WRL::ComPtr;

namespace rrv
{

	struct FrameCBuffer
	{
		XMFLOAT4X4 view;
		XMFLOAT4X4 proj;
		XMFLOAT3   lightDir;
		float      padding;
	};

	struct RenderFrame
	{
		ComPtr<ID3D12CommandAllocator> cmd_alloc;
		UINT64 fence_value = 0;

		rrv::ConstantBufferSlice constantBufSlice;
		rrv::InstanceBufferSlice instanceBufSlice;
	};

	struct InstanceData
	{
		XMFLOAT4X4 world;
	};


	struct Renderer::Impl
	{
		static constexpr UINT MAX_INSTANCE_COUNT = 2048;
	public:
		int width = 0;
		int height = 0;

		// Core Device
		ComPtr<IDXGIFactory6>      factory;
		ComPtr<IDXGIAdapter1>      adapter;
		ComPtr<ID3D12Device5>      device;
		ComPtr<ID3D12CommandQueue> cmdQueue;

		// Swap Chain
		ComPtr<IDXGISwapChain3>    swapChain;
		BOOL 					   tearingSupported = FALSE;

		// Descriptor Heaps
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12DescriptorHeap> srvHeap;
		ComPtr<ID3D12DescriptorHeap> dsvHeap;
		UINT rtvDescriptorSize = 0;
		UINT srvDescriptorSize = 0;
		UINT dsvDescriptorSize = 0;

		// Render Targets / Depth Buffer
		ComPtr<ID3D12Resource> renderTargets[rrv::FRAME_COUNT];
		ComPtr<ID3D12Resource> depthBuffer;

		// Global Constant Buffer
		rrv::ConstantBuffer globalConstantBuffer;
		rrv::InstanceBuffer globalInstanceBuffer;

		// Per-Frame Resources
		RenderFrame    m_renderFrames[rrv::FRAME_COUNT];
		FrameCBuffer   frameCBuffer[rrv::FRAME_COUNT];
		UINT           frameIndex = 0;


		// Sync Objects
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		rrv::GpuFence gpuFence;
		rrv::GpuEvent gpuEvent;

		// TODO: need refactoring
		// Mesh Resources
		ComPtr<ID3D12Resource> vertexPositionBuffer;
		ComPtr<ID3D12Resource> vertexNormalUVBuffer;
		ComPtr<ID3D12Resource> indexBuffer;

		D3D12_VERTEX_BUFFER_VIEW vertexPositionBufferView = {};
		D3D12_VERTEX_BUFFER_VIEW vertexNormalUVBufferView = {};
		D3D12_INDEX_BUFFER_VIEW  indexBufferView = {};

		UINT vertexCount = 0;
		UINT  indexCount = 0;


		// Texture Resources
		ComPtr<ID3D12Resource> textureResource;
		
		std::vector<ComPtr<ID3D12Resource>> m_textures;

		// Pipeline
		Pipeline m_pipeline;

		// uploader
		std::unique_ptr<rrv::GpuUploader> m_gpuUploader;
	};

	Renderer::Renderer() : m_impl(std::make_unique<Impl>()) {}
	Renderer::~Renderer() = default;

	bool Renderer::Init(void* hwnd, int width, int height)
	{
		auto& d = *m_impl;

		d.width = width;
		d.height = height;

	#ifdef _DEBUG
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
	#endif

		// Create DXGI Factory
		UINT factoryFlags = 0;
	#ifdef _DEBUG
		factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
	#endif
		CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&d.factory));

		// Select the start available hardware adapter that supports Direct3D 12
		for (UINT i = 0;
			d.factory->EnumAdapterByGpuPreference(i,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(&d.adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			if (SUCCEEDED(D3D12CreateDevice(d.adapter.Get(),
				D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d.device))))
				break;
		}

		// Create a command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		THROW_IF_FAILED(d.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d.cmdQueue)));

		// Create a command list
		THROW_IF_FAILED(d.device->CreateCommandList1(0,
			D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&d.cmdList)));


		d.gpuFence = rrv::GpuFence(d.device.Get());
		d.gpuEvent = rrv::GpuEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));


		// Check tearing supported
		d.factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &d.tearingSupported, sizeof(d.tearingSupported));


		// Create a swap chain
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;
		scDesc.Height = height;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.BufferCount = rrv::FRAME_COUNT;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.SampleDesc = { 1, 0 };
		scDesc.Flags = d.tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> swapChain1;
		THROW_IF_FAILED(d.factory->CreateSwapChainForHwnd(
			d.cmdQueue.Get(), static_cast<HWND>(hwnd),
			&scDesc, nullptr, nullptr,
			&swapChain1
		));

		// cast as IDXGISwapChain3
		THROW_IF_FAILED(swapChain1.As(&d.swapChain));


		d.m_pipeline.Init(d.device.Get());
		d.globalConstantBuffer = rrv::ConstantBuffer(d.device.Get(), 4096);
		d.globalInstanceBuffer = rrv::InstanceBuffer(d.device.Get(), sizeof(InstanceData), d.MAX_INSTANCE_COUNT);
		d.m_gpuUploader = std::make_unique<rrv::GpuUploader>(d.device.Get());



		d.rtvDescriptorSize = d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		d.srvDescriptorSize = d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		d.dsvDescriptorSize = d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);



		// Create RTV heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = rrv::FRAME_COUNT;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			THROW_IF_FAILED(d.device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&d.rtvHeap)));
		}

		// Create DSV heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			THROW_IF_FAILED(d.device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&d.dsvHeap)));
		}

		// Create SRV heap
		{
			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = rrv::RenderConfig::SRV_MAX_COUNT;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			THROW_IF_FAILED(d.device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&d.srvHeap)));
		}

		// Init srv heap with null descriptors
		{
			auto nullDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1);

			CD3DX12_CPU_DESCRIPTOR_HANDLE handleBase(d.srvHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < rrv::RenderConfig::SRV_MAX_COUNT; ++i)
			{
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(handleBase, i, d.srvDescriptorSize);
				d.device->CreateShaderResourceView(nullptr, &nullDesc, handle);
			}
		}



		// Create RTV for each back buffer
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE handleBase(d.rtvHeap->GetCPUDescriptorHandleForHeapStart());
			for (UINT i = 0; i < rrv::FRAME_COUNT; ++i)
			{
				auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(handleBase, i, d.rtvDescriptorSize);
				THROW_IF_FAILED(d.swapChain->GetBuffer(i, IID_PPV_ARGS(&d.renderTargets[i])));
				d.device->CreateRenderTargetView(d.renderTargets[i].Get(), nullptr, handle);
			}
		}


		// Create depth buffer and DSV descriptor
		{
			CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height);
			depthDesc.MipLevels = 1;
			depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

			auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			THROW_IF_FAILED(d.device->CreateCommittedResource(
				&heapProps, D3D12_HEAP_FLAG_NONE,
				&depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthClearValue, IID_PPV_ARGS(&d.depthBuffer)));

			d.device->CreateDepthStencilView(
				d.depthBuffer.Get(), nullptr,
				d.dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}


		// Create SRV for instance buffer
		{
			auto instanceSrvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(
				d.MAX_INSTANCE_COUNT, sizeof(InstanceData));

			CD3DX12_CPU_DESCRIPTOR_HANDLE instanceSrvHandle(d.srvHeap->GetCPUDescriptorHandleForHeapStart(),
				rrv::RenderConfig::SRV_INSTANCE_DATA_START,
				d.srvDescriptorSize);

			d.device->CreateShaderResourceView(
				d.globalInstanceBuffer.GetResource(), &instanceSrvDesc, instanceSrvHandle);
		}

		// Create frame resources
		for (int i = 0; i < rrv::FRAME_COUNT; i++)
		{
			auto& frameResource = d.m_renderFrames[i];
			THROW_IF_FAILED(d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&frameResource.cmd_alloc)));

			frameResource.constantBufSlice = d.globalConstantBuffer.Allocate(sizeof(FrameCBuffer));
			frameResource.instanceBufSlice = d.globalInstanceBuffer.Allocate(100);
		}

		return true;
	}

	void Renderer::Shutdown()
	{
		auto& d = *m_impl;

		// Wait for GPU to finish
		d.gpuFence.Signal(d.cmdQueue.Get());
		d.gpuFence.Wait(d.gpuEvent);
	}

	void Renderer::SetCamera(rrv::Vec3 eyePos, rrv::Vec3 focusPos)
	{
		auto& d = *m_impl;
		XMVECTOR xmEyePos = XMVectorSet(eyePos.x, eyePos.y, eyePos.z, 1.0f);
		XMVECTOR xmFocusPos = XMVectorSet(focusPos.x, focusPos.y, focusPos.z, 1.0f);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		auto& perFrameCBData = d.frameCBuffer[d.frameIndex];
		XMStoreFloat4x4(&perFrameCBData.view, XMMatrixTranspose(XMMatrixLookAtLH(xmEyePos, xmFocusPos, up)));
	}

	void Renderer::Render()
	{
		auto& d = *m_impl;
		auto& frameResource = d.m_renderFrames[d.frameIndex];
		auto& perFrameCBData = d.frameCBuffer[d.frameIndex];

		// Wait for GPU to finish with the current frame resource
		d.gpuFence.Wait(d.gpuEvent, frameResource.fence_value);

		// Reset allocator and command list
		frameResource.cmd_alloc->Reset();
		d.cmdList->Reset(frameResource.cmd_alloc.Get(), nullptr);

		// Transition back buffer: Present -> RenderTarget
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(d.renderTargets[d.frameIndex].Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

			d.cmdList->ResourceBarrier(1, &barrier);
		}

		// Set viewport and scissor rect
		{
			D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(d.width), static_cast<float>(d.height), 0.0f, 1.0f };
			D3D12_RECT scissorRect = { 0, 0, d.width, d.height };
			d.cmdList->RSSetViewports(1, &viewport);
			d.cmdList->RSSetScissorRects(1, &scissorRect);
		}

		// Clear RTV
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(d.rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			d.frameIndex, d.rtvDescriptorSize);

		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		d.cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// Clear DSV
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d.dsvHeap->GetCPUDescriptorHandleForHeapStart());
		d.cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Set render target and depth buffer
		d.cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Set pipeline state and root signature
		d.cmdList->SetPipelineState(d.m_pipeline.GetStaticMeshPso());
		d.cmdList->SetGraphicsRootSignature(d.m_pipeline.GetGlobalRootSignature());

		XMFLOAT3 lightDir = { 0.5f, -1.0f, 0.5f };
		// normalize light direction
		XMVECTOR ld = XMVector3Normalize(XMLoadFloat3(&lightDir));
		XMStoreFloat3(&perFrameCBData.lightDir, ld);

		// Projection matrix
		float aspectRatio = (float)d.width / (float)d.height;
		XMStoreFloat4x4(&perFrameCBData.proj, XMMatrixTranspose(
			XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspectRatio, 0.1f, 1000.0f)));

		memcpy(frameResource.constantBufSlice.mapped, &perFrameCBData, sizeof(perFrameCBData));

		// Bind Per-Frame Constant Buffer
		d.cmdList->SetGraphicsRootConstantBufferView(0, frameResource.constantBufSlice.gpuAddress);

		// Bind SRV heap
		ID3D12DescriptorHeap* heaps[] = { d.srvHeap.Get() };
		d.cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
		d.cmdList->SetGraphicsRootDescriptorTable(3, d.srvHeap->GetGPUDescriptorHandleForHeapStart());

		// Draw
		d.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d.cmdList->IASetIndexBuffer(&d.indexBufferView);
		d.cmdList->IASetVertexBuffers(0, 1, &d.vertexPositionBufferView);
		d.cmdList->IASetVertexBuffers(1, 1, &d.vertexNormalUVBufferView);

		d.cmdList->DrawIndexedInstanced(d.indexCount, frameResource.instanceBufSlice.count, 0, 0, frameResource.instanceBufSlice.index);

		// Transition back buffer: RenderTarget -> Present
		{
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				d.renderTargets[d.frameIndex].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);
			d.cmdList->ResourceBarrier(1, &barrier);
		}

		d.cmdList->Close();

		// Execute
		ID3D12CommandList* lists[] = { d.cmdList.Get() };
		d.cmdQueue->ExecuteCommandLists(1, lists);


		// Signal
		frameResource.fence_value = d.gpuFence.Signal(d.cmdQueue.Get());

		// Present
		d.swapChain->Present(0, d.tearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0);
		d.frameIndex = d.swapChain->GetCurrentBackBufferIndex();

	}

	void Renderer::Test_UpdateInstances(float deltaTime)
	{
		//TODO: need to double buffering

		auto& d = *m_impl;
		auto& frameResource = d.m_renderFrames[d.frameIndex];

		// Update instance buffer
		static float angle = 0.0f;
		angle += deltaTime;
		// write instance data, 10 * 10 grid
		for (uint32_t i = 0; i < frameResource.instanceBufSlice.count / 10; ++i)
		{
			for (uint32_t j = 0; j < 10; ++j)
			{
				InstanceData data = {};
				XMMATRIX world = XMMatrixScaling(0.1f, 0.1f, 0.1f)
					* XMMatrixRotationY(angle)
					* XMMatrixTranslation(i * 20.0f, 0.0f, j * 20.0f);

				XMStoreFloat4x4(&data.world, XMMatrixTranspose(world));


				memcpy(frameResource.instanceBufSlice.mapped + size_t(i * 10 + j) * sizeof(data),
					&data, sizeof(data));
			}
		}
	}

	void Renderer::Test_UploadDDSTexture()
	{
	}
}

