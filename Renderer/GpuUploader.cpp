#include "pch.h"
#include "GpuUploader.h"
#include "DxUtils.h"

namespace rrv
{
	GpuUploader::~GpuUploader()
	{
		m_shutdown = true;
		m_cv.notify_one();
		m_thread.join();
	}

	GpuUploader::GpuUploader(ID3D12Device5* device, uint64_t init_upload_buffer_size)
		: m_device(device)
		, m_fence(device)
		, m_event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
	{
		// Create Copy Command Objects
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		THROW_IF_FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_cmd_queue)));

		THROW_IF_FAILED(device->CreateCommandList1(0,
			D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&m_cmd_list)));

		// Create Upload Frame Resources
		for (int i = 0; i < FRAME_COUNT; ++i)
		{
			THROW_IF_FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
				IID_PPV_ARGS(&m_frames[i].cmd_alloc)));

			m_frames[i].buffer = GpuUploadBuffer(device, init_upload_buffer_size);
		}

		m_thread = std::thread(&GpuUploader::WorkerLoop, this);
	}

	void GpuUploader::WaitForUploadComplete(GpuEvent& event, uint64_t fence_value)
	{
		m_fence.Wait(event, fence_value);
	}

	void GpuUploader::WaitForUploadComplete(GpuEvent& event)
	{
		m_fence.Wait(event, m_upload_count);
	}

	uint64_t GpuUploader::EnqueueUploadBuffer(ID3D12Resource* dst_resource, uint64_t dst_offset, std::vector<uint8_t> src_data)
	{
		uint64_t upload_id = UINT64_MAX;
		{
			std::lock_guard lock(m_mutex);
			upload_id = ++m_upload_count;

			GpuUploadTask task = {};
			task.dst_resource = dst_resource;
			task.dst_offset = dst_offset;
			task.src_data = std::move(src_data);
			task.upload_id = upload_id;
			task.type = GpuUploadTask::Type::BUFFER;

			m_pending_tasks.push_back(std::move(task));
		}
		m_cv.notify_one();


		return upload_id;
	}

	uint64_t GpuUploader::EnqueueUploadTexture(ID3D12Resource* dst_resource, DirectX::ScratchImage src_image)
	{
		uint64_t upload_id;
		{
			std::lock_guard lock(m_mutex);
			upload_id = ++m_upload_count;

			GpuUploadTask task = {};
			task.dst_resource = dst_resource;
			task.src_image = std::move(src_image);
			task.upload_id = upload_id;
			task.type = GpuUploadTask::Type::TEXTURE;

			m_pending_tasks.push_back(std::move(task));
		}
		m_cv.notify_one();

		return upload_id;
	}

	void GpuUploader::WorkerLoop()
	{
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		while (!m_shutdown)
		{
			{
				std::unique_lock lock(m_mutex);
				m_cv.wait(lock, [this] {
					return !m_pending_tasks.empty() || m_shutdown;
					});

				if (m_shutdown) break;

				// m_active_tasks must be empty, because the worker is not busy
				std::swap(m_pending_tasks, m_active_tasks);
			}

			// worker busy
			ReadyForNextFrame();
			for (auto it = m_active_tasks.begin(); it != m_active_tasks.end(); ++it)
			{
				subresources.clear();
				uint64_t upload_data_size;
				
				if (it->type == GpuUploadTask::Type::TEXTURE)
				{
					THROW_IF_FAILED(DirectX::PrepareUpload(m_device, 
						it->src_image.GetImages(), 
						it->src_image.GetImageCount(), 
						it->src_image.GetMetadata(),
						subresources));
			
					upload_data_size = GetRequiredIntermediateSize(it->dst_resource, 
						0, static_cast<uint32_t>(subresources.size()));
				}
				else
				{
					upload_data_size = it->src_data.size();
				}

				// if the data size is larger than the upload buffer size,
				// flush command queue
				// and recreate upload buffer with larger size
				if (upload_data_size > GetCurrentFrame().buffer.GetSize())
				{
					if (0 < GetCurrentFrame().buffer_offset)
					{
						Submit();
						ReadyForNextFrame();
					}

					GetCurrentFrame().buffer = GpuUploadBuffer(m_device, upload_data_size);
				}

				if (upload_data_size + GetCurrentFrame().buffer_offset > GetCurrentFrame().buffer.GetSize())
				{
					assert(0 < GetCurrentFrame().buffer_offset);
					Submit();
					ReadyForNextFrame();
				}

				// record cmd list 
				if (it->type == GpuUploadTask::Type::TEXTURE)
				{
					UpdateSubresources(m_cmd_list.Get(),
						it->dst_resource, 
						GetCurrentFrame().buffer.Get(),
						GetCurrentFrame().buffer_offset, 
						0, static_cast<unsigned int>(subresources.size()),
						subresources.data());

					//release src_image
					it->src_image.Release();

				}
				else
				{
					memcpy(GetCurrentFrame().buffer.GetMapped() + GetCurrentFrame().buffer_offset,
						it->src_data.data(), 
						upload_data_size);

					//release src_data
					it->src_data.clear();

					m_cmd_list->CopyBufferRegion(
						it->dst_resource, it->dst_offset,
						GetCurrentFrame().buffer.Get(), GetCurrentFrame().buffer_offset,
						upload_data_size);
				}

				GetCurrentFrame().buffer_offset += AlignUp(upload_data_size, u64(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
				m_submit_signal = it->upload_id;
			}

			Submit();
			m_active_tasks.clear();

			// worker not busy
		}
	}

	void GpuUploader::Submit()
	{
		m_cmd_list->Close();
		ID3D12CommandList* cmd_lists[] = { m_cmd_list.Get() };
		m_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);

		GetCurrentFrame().fence_value = m_fence.Signal(m_cmd_queue.Get(), m_submit_signal);
	}
	void GpuUploader::ReadyForNextFrame()
	{
		m_frame_idx = (m_frame_idx + 1) % FRAME_COUNT;
		m_fence.Wait(m_event, GetCurrentFrame().fence_value);
		GetCurrentFrame().buffer_offset = 0;
		GetCurrentFrame().cmd_alloc->Reset();
		m_cmd_list->Reset(GetCurrentFrame().cmd_alloc.Get(), nullptr);
	}
}

