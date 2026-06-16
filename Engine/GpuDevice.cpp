#include "pch.h"
#include "GPUDevice.h"

namespace rr
{
	static ComPtr<IDXGIAdapter1> GetHighPerformanceAdapter(ComPtr<IDXGIFactory6>& factory)
	{
		ComPtr<IDXGIAdapter1> adapter;
		RR_D3D_CHECK(factory->EnumAdapterByGpuPreference(
			0,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&adapter)
		));

		return adapter;
	}

	GPUDevice::GPUDevice(HWND hwnd)
		: hwnd_(hwnd)
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
		::CreateDXGIFactory2(0, IID_PPV_ARGS(&factory_));
		auto adapter = GetHighPerformanceAdapter(factory_);
		D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
		RR_D3D_CHECK(::D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&device_)));

		// [DEBUG] Enable breaking on D3D12 errors and corruption
	#ifdef RR_D3D12_DEBUG
		ComPtr<ID3D12InfoQueue1> info_queue;
		if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&info_queue))))
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
		RR_D3D_CHECK(device_->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&queue_)));

		// Check for tearing support
		BOOL allow_tearing = FALSE;
		factory_->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&allow_tearing,
			sizeof(allow_tearing));

		allow_tearing_ = (allow_tearing == TRUE);

		// Create the swapchain
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
		swapchain_desc.BufferCount = kBackBufferCount;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.Flags = allow_tearing_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> swapchain1;
		RR_D3D_CHECK(factory_->CreateSwapChainForHwnd(
			queue_.Get(),
			hwnd_,
			&swapchain_desc,
			nullptr,
			nullptr,
			&swapchain1
		));
		RR_D3D_CHECK(swapchain1.As(&swapchain_));

		// Disable the ALT+ENTER fullscreen toggle feature
		RR_D3D_CHECK(factory_->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

		// Create the fence for flushing the command queue
		fence_value_ = 0;
		RR_D3D_CHECK(device_->CreateFence(fence_value_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));

		// Create the event for waiting on the fence
		fence_event_ = SimpleEvent(::CreateEventW(nullptr, FALSE, FALSE, nullptr));


		// Create the RTV descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NumDescriptors = kBackBufferCount;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heap_desc.NodeMask = 0;
		RR_D3D_CHECK(device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap_)));

		rtv_handle_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	void GPUDevice::Present()
	{
		const UINT flags = (allow_tearing_) ? DXGI_PRESENT_ALLOW_TEARING : 0;
		swapchain_->Present(0, flags);
	}

	void GPUDevice::Flush()
	{
		const uint64_t fence_value = ++fence_value_;
		RR_D3D_CHECK(queue_->Signal(fence_.Get(), fence_value));
		if (fence_->GetCompletedValue() < fence_value)
		{
			RR_D3D_CHECK(fence_->SetEventOnCompletion(fence_value, fence_event_.Get()));
			::WaitForSingleObject(fence_event_.Get(), INFINITE);
		}
	}
	void GPUDevice::Resize(uint32_t, uint32_t)
	{
		Flush();
		for (auto& back_buffer : rtv_resources_)
		{
			back_buffer.Reset();
		}


		RR_D3D_CHECK(swapchain_->ResizeBuffers(
			kBackBufferCount, 0, 0, 
			DXGI_FORMAT_R8G8B8A8_UNORM, 
			allow_tearing_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0));
		
		CreateRenderTargets();
	}
	void GPUDevice::CreateRenderTargets()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());
		for (int i = 0; i < kBackBufferCount; ++i)
		{
			RR_D3D_CHECK(swapchain_->GetBuffer(i, IID_PPV_ARGS(&rtv_resources_[i])));
			device_->CreateRenderTargetView(rtv_resources_[i].Get(), nullptr, handle);
			rtv_handles_[i] = handle;
			handle.Offset(1, rtv_handle_size_);
		}
	}
}