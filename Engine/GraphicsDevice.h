#pragma once
#include "SimpleEvent.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

namespace rr
{
	using Microsoft::WRL::ComPtr;

	class GraphicsDevice
	{
	public:
		static constexpr unsigned int kBackBufferCount = 2;
	public:
		GraphicsDevice() = default;
		~GraphicsDevice() noexcept = default;
		GraphicsDevice(GraphicsDevice&&) noexcept = default;
		GraphicsDevice& operator=(GraphicsDevice&&) noexcept = default;
		//Non copyable
		GraphicsDevice(GraphicsDevice const&) = delete;
		GraphicsDevice& operator=(GraphicsDevice const&) = delete;

		GraphicsDevice(HWND hwnd);
	public:
		ID3D12Device5* GetDevice() const noexcept { return m_device.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const noexcept { return m_cmd_queue.Get(); }
		uint32_t GetBackBufferIndex() const { return m_swapchain->GetCurrentBackBufferIndex(); }

		void Present();
		void Flush();
		void Resize(uint32_t, uint32_t);

	private:
		void CreateRenderTargets();

	private:
		HWND m_hwnd = nullptr;
		ComPtr<IDXGIFactory6> m_factory;
		ComPtr<ID3D12Device5> m_device;
		ComPtr<ID3D12CommandQueue> m_cmd_queue;
		ComPtr<IDXGISwapChain3>    m_swapchain;

		uint64_t                   m_fence_value = 0;
		ComPtr<ID3D12Fence>        m_fence;
		SimpleEvent                m_fence_event;

		ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
		ComPtr<ID3D12Resource>       m_rtv_resources[kBackBufferCount];
		D3D12_CPU_DESCRIPTOR_HANDLE  m_rtv_handles[kBackBufferCount] = {};
		uint32_t                     m_rtv_increment_size = 0;

		bool m_tearing_support = false;
	};

}