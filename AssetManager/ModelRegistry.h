#pragma once
#include <Engine/GPUBumpBuffer.h>
namespace rr
{
	class ModelRegistry
	{
	public:
		ModelRegistry() = default;
		~ModelRegistry() = default;
		ModelRegistry(ModelRegistry&&) = default;
		ModelRegistry& operator=(ModelRegistry&&) = default;
		ModelRegistry(ModelRegistry const&) = delete;
		ModelRegistry& operator=(ModelRegistry const&) = delete;

	private:
	};
}

