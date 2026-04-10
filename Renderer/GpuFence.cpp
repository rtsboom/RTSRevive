#include "pch.h"
#include "GpuFence.h"
#include "DxUtils.h"
namespace rrv
{
	GpuFence::GpuFence(ID3D12Device* device, uint64_t init_signal)
		: m_last_signal(init_signal)
	{
		THROW_IF_FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	}


	uint64_t GpuFence::Signal(ID3D12CommandQueue* cmd_queue, uint64_t signal)
	{
		// the signal value must be greater than the count signal value
		if (signal <= m_last_signal) 
			return 0;

		cmd_queue->Signal(m_fence.Get(), signal);
		m_last_signal = signal;

		return signal;
	}

	uint64_t GpuFence::Signal(ID3D12CommandQueue* cmd_queue)
	{
		cmd_queue->Signal(m_fence.Get(), ++m_last_signal);
		return m_last_signal;
	}

	void GpuFence::Wait(GpuEvent& event, uint64_t value)
	{
		assert(event.Get());
		assert(value < UINT64_MAX);

		if (m_fence->GetCompletedValue() < value)
		{
			THROW_IF_FAILED(m_fence->SetEventOnCompletion(value, event.Get()));
			WaitForSingleObject(event.Get(), INFINITE);
		}
	}
	void GpuFence::Wait(GpuEvent& event)
	{
		Wait(event, m_last_signal);
	}
}