#pragma once
#include "GpuUploadBuffer.h"
#include "GpuFence.h"

#include <DirectXTex.h>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace rrv
{
	using Microsoft::WRL::ComPtr;
	using DirectX::ScratchImage;

	struct GpuUploadTask
	{
		enum class Type { BUFFER, TEXTURE };

		ID3D12Resource* dst_resource;
		uint64_t		dst_offset;
		std::vector<uint8_t>  src_data;
		DirectX::ScratchImage src_image;
		uint64_t upload_id;
		Type     type;
	};

	struct GpuUploadFrame
	{
		ComPtr<ID3D12CommandAllocator> cmd_alloc;
		GpuUploadBuffer buffer;
		uint64_t	    buffer_offset = 0;
		uint64_t        fence_value = 0;
	};

	class GpuUploader
	{
	public:
		static constexpr int FRAME_COUNT = 2;
		~GpuUploader();
		GpuUploader() = default;

		// no copy and move
		GpuUploader(GpuUploader&&) = delete;
		GpuUploader& operator=(GpuUploader&&) = delete;
		GpuUploader(const GpuUploader&) = delete;
		GpuUploader& operator=(const GpuUploader&) = delete;
	public:
		GpuUploader(ID3D12Device5* device, uint64_t init_upload_buffer_size = 64 * 1024);

		void WaitForUploadComplete(GpuEvent& event, uint64_t upload_id);
		void WaitForUploadComplete(GpuEvent& event);
		bool CheckUploadComplete(uint64_t upload_id) const { return upload_id <= m_fence.GetCompletedValue(); }

		uint64_t EnqueueUploadBuffer(ID3D12Resource* dst_resource, uint64_t dst_offset, std::vector<uint8_t> src_data);
		uint64_t EnqueueUploadTexture(ID3D12Resource* dst_resource, DirectX::ScratchImage image);

	private:
		void WorkerLoop();
		void Submit();
		void ReadyForNextFrame();
		GpuUploadFrame& GetCurrentFrame() { return m_frames[m_frame_idx]; }
	private:
		ID3D12Device5* m_device = nullptr;

		ComPtr<ID3D12CommandQueue>		  m_cmd_queue;
		ComPtr<ID3D12GraphicsCommandList> m_cmd_list;

		GpuFence m_fence;
		GpuEvent m_event;

		GpuUploadFrame m_frames[FRAME_COUNT];
		uint64_t	   m_frame_idx = 0;
		uint64_t	   m_submit_signal = 0;

		std::vector<GpuUploadTask> m_pending_tasks;
		std::vector<GpuUploadTask> m_active_tasks;


		std::atomic<uint64_t> m_upload_count = 0;
		std::atomic<bool>     m_shutdown = false;

		std::condition_variable m_cv;
		std::mutex				m_mutex;
		std::thread				m_thread;
	};

}
