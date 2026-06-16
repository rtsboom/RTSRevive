#include "pch.h"
#include "GPURegistry.h"
#include "GPUBumpBuffer.h"
#include <CastUtils.h>
#include <utility>
#include <cstdint>

namespace rr
{
	uint64_t GPURegistry::AddBuffer(GPUBumpBuffer&& gpu_buffer) noexcept
	{
		uint64_t handle = buffers_.size();
		buffers_.push_back(std::move(gpu_buffer));

		return handle;
	}

	GPUBumpBuffer& GPURegistry::GetBuffer(uint64_t handle) noexcept
	{
		RR_CHECK(handle < buffers_.size());
		return buffers_[handle];
	}
}