#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <string>
namespace rr
{
	class AssetLoader
	{
		enum class LoadType : uint32_t
		{
			Texture,
			Model,
			Animation,
			Count
		};

		enum class LoadState : uint32_t
		{
			Idle,
			Requested,
			Loading,
			Loaded,
			Failed
		};

		struct alignas(64) LoadSlot
		{
			std::atomic<LoadState> state;
			LoadType type;
			std::string path;
			void* result;
		};

	public:
		AssetLoader() = default;
		~AssetLoader() = default;
		AssetLoader(AssetLoader const&) = delete;
		AssetLoader& operator=(AssetLoader const&) = delete;
		AssetLoader(AssetLoader&&) = delete;
		AssetLoader& operator=(AssetLoader&&) = delete;

	private:
		std::atomic<bool> is_running_;
		std::mutex mutex_;

	};
}

