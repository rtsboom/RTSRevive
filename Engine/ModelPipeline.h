#pragma once
#include <Engine/ModelAsset.h>
#include <Engine/FileBatch.h>
#include <Engine/StableArray.h>
#include <TinyGLTFv3/tiny_gltf_v3.h>
#include <bitset>
#include <cstdint>

namespace rr
{
	class Job;
	class JobSystem;
	class ModelPipeline
	{
		using ParsedModel = tg3_model;
		static constexpr size_t kParsedModelCount = 32;
	public:
		ModelPipeline(ModelPipeline const&) = delete;
		ModelPipeline& operator=(ModelPipeline const&) = delete;
		ModelPipeline(ModelPipeline&&) = default;
		ModelPipeline& operator=(ModelPipeline&&) = default;


	public:
		void Parse(FileBatch* file_batch);
		void Convert(JobSystem* job_system);

	private:
		ParsedModel parsed_models_[kParsedModelCount];
		std::bitset<kParsedModelCount> parsed_model_mask_;
		StableArray<Model> engine_models_;

	};
}

