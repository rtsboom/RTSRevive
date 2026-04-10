#pragma once
#include "GpuEvent.h"

#include <d3d12.h>
#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>

namespace rrv
{
	using Microsoft::WRL::ComPtr;

	class GpuFence
	{
	public:
		GpuFence() = default;
		~GpuFence() noexcept = default;
		GpuFence(GpuFence&&) noexcept = default;
		GpuFence& operator=(GpuFence&&) noexcept = default;
		GpuFence(const GpuFence&) = delete;
		GpuFence& operator=(const GpuFence&) = delete;

	public:
		GpuFence(ID3D12Device* device, uint64_t init_signal = 0);
	public:
		uint64_t Signal(ID3D12CommandQueue* cmd_queue, uint64_t signal);
		uint64_t Signal(ID3D12CommandQueue* cmd_queue);
		void Wait(GpuEvent& event, uint64_t value);
		void Wait(GpuEvent& event);
		uint64_t GetCompletedValue() const { return m_fence->GetCompletedValue(); }
		uint64_t GetLastSignal() const { return m_last_signal; }


	private:
		ComPtr<ID3D12Fence> m_fence;
		uint64_t            m_last_signal = 0;
	};
}

