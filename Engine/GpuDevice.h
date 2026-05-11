#pragma once
#include "SimpleEvent.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

namespace rr
{
	using Microsoft::WRL::ComPtr;

	class GPUDevice
	{
	public:
		static constexpr unsigned int kBackBufferCount = 2;
	public:
		GPUDevice() = default;
		~GPUDevice() noexcept = default;
		GPUDevice(GPUDevice&&) noexcept = default;
		GPUDevice& operator=(GPUDevice&&) noexcept = default;
		//Non copyable
		GPUDevice(GPUDevice const&) = delete;
		GPUDevice& operator=(GPUDevice const&) = delete;

		GPUDevice(HWND hwnd);
	public:
		ID3D12Device5* GetDevice() const noexcept { return device_.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const noexcept { return queue_.Get(); }
		uint32_t GetBackBufferIndex() const { return swapchain_->GetCurrentBackBufferIndex(); }

		void Present();
		void Flush();
		void Resize(uint32_t, uint32_t);

	private:
		void CreateRenderTargets();

	private:
		HWND hwnd_{};
		ComPtr<IDXGIFactory6> factory_{};
		ComPtr<ID3D12Device5> device_{};
		ComPtr<ID3D12CommandQueue> queue_{};
		ComPtr<IDXGISwapChain3>    swapchain_{};

		uint64_t                   fence_value_{};
		ComPtr<ID3D12Fence>        fence_{};
		SimpleEvent                fence_event_{};

		ComPtr<ID3D12DescriptorHeap> rtv_heap_{};
		ComPtr<ID3D12Resource>       rtv_resources_[kBackBufferCount]{};
		D3D12_CPU_DESCRIPTOR_HANDLE  rtv_handles_[kBackBufferCount]{};
		uint32_t                     rtv_handle_size_{};

		bool allow_tearing_ = false;
	};

}