#include "EnginePch.h"
#include "GpuDevice.h"

namespace rr
{
	static ComPtr<IDXGIAdapter1> GetHighPerformanceAdapter(IDXGIFactory6* factory)
	{
		ComPtr<IDXGIAdapter1> adapter;
		THROW_IF_FAILED(factory->EnumAdapterByGpuPreference(
			0,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&adapter)
		));

		return adapter;
	}

	GpuDevice::GpuDevice(HWND hwnd)
		: m_hwnd(hwnd)
	{
		// [DEBUG] Enable debug interface
	#ifdef RR_D3D12_DEBUG
		ComPtr<ID3D12Debug> debug_controller;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
		{
			debug_controller->EnableDebugLayer();
		}
	#endif

		// Create the DXGI factory and the D3D12 device
		::CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
		auto adapter = GetHighPerformanceAdapter(m_factory.Get());
		D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
		THROW_IF_FAILED(::D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&m_device)));

		// [DEBUG] Enable breaking on D3D12 errors and corruption
	#ifdef RR_D3D12_DEBUG
		ComPtr<ID3D12InfoQueue1> info_queue;
		if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&info_queue))))
		{
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	#endif

		// Create the command queue
		D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
		cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cmd_queue_desc.NodeMask = 0;
		THROW_IF_FAILED(m_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&m_cmd_queue)));

		// Check for tearing support
		BOOL allow_tearing = {};
		m_factory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&allow_tearing,
			sizeof(allow_tearing));

		m_tearing_support = (allow_tearing == TRUE);

		// Create the swapchain
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
		swapchain_desc.BufferCount = kBackBufferCount;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.Flags = m_tearing_support ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> swapchain1;
		THROW_IF_FAILED(m_factory->CreateSwapChainForHwnd(
			m_cmd_queue.Get(),
			m_hwnd,
			&swapchain_desc,
			nullptr,
			nullptr,
			&swapchain1
		));
		THROW_IF_FAILED(swapchain1.As(&m_swapchain));

		// Disable the ALT+ENTER fullscreen toggle feature
		THROW_IF_FAILED(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

		// Create the fence for flushing the command queue
		m_fence_value = 0;
		THROW_IF_FAILED(m_device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

		// Create the event for waiting on the fence
		m_fence_event = SimpleEvent(::CreateEventW(nullptr, FALSE, FALSE, nullptr));


		// Create the RTV descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NumDescriptors = kBackBufferCount;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heap_desc.NodeMask = 0;
		THROW_IF_FAILED(m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&m_rtv_heap)));

		m_rtv_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void GpuDevice::Present()
	{
		const UINT flags = (m_tearing_support) ? DXGI_PRESENT_ALLOW_TEARING : 0;
		m_swapchain->Present(0, flags);
	}

	void GpuDevice::Flush()
	{
		const uint64_t fence_value = ++m_fence_value;
		THROW_IF_FAILED(m_cmd_queue->Signal(m_fence.Get(), fence_value));
		if (m_fence->GetCompletedValue() < fence_value)
		{
			THROW_IF_FAILED(m_fence->SetEventOnCompletion(fence_value, m_fence_event.Get()));
			::WaitForSingleObject(m_fence_event.Get(), INFINITE);
		}
	}
	void GpuDevice::Resize(uint32_t, uint32_t)
	{
		Flush();
		for (auto& back_buffer : m_rtv_resources)
		{
			back_buffer.Reset();
		}


		THROW_IF_FAILED(m_swapchain->ResizeBuffers(
			kBackBufferCount, 0, 0, 
			DXGI_FORMAT_R8G8B8A8_UNORM, 
			m_tearing_support ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0));
		
		CreateRenderTargets();
	}
	void GpuDevice::CreateRenderTargets()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());
		for (int i = 0; i < kBackBufferCount; ++i)
		{
			THROW_IF_FAILED(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_rtv_resources[i])));
			m_device->CreateRenderTargetView(m_rtv_resources[i].Get(), nullptr, handle);
			m_rtv_handles[i] = handle;
			handle.Offset(1, m_rtv_increment_size);
		}
	}
}