#include "pch.h"
#include <Engine/Asserts.h>
#include <Engine/FileBatch.h>
#include <Engine/JobSystem.h>
#include <Engine/Timer.h>
#include <TinyGLTFv3/tiny_gltf_v3.h>
#include <filesystem>
#include <string_view>
namespace
{
	using namespace rr;
	void LoadGltfFromFileBlob(rr::FileBlob const& file_blob)
	{
		size_t pos = file_blob.path.find_last_of("/\\");
		std::string_view base_dir = {};
		if (pos != std::string_view::npos)
		{
			base_dir = file_blob.path.substr(0, pos);
		}
		base_dir = {};

		tg3_model model;
		tg3_parse_options opts;
		tg3_error_stack errors;
		tg3_parse_options_init(&opts);
		tg3_error_stack_init(&errors);
		opts.images_as_is = 1;
		opts.validate_indices = 1;
		opts.parse_float32 = 1;
		tg3_error_code result = tg3_parse_auto(
			&model,
			&errors,
			reinterpret_cast<uint8_t const*>(file_blob.bytes.data()),
			file_blob.bytes.size(),
			base_dir.data(),
			static_cast<uint32_t>(base_dir.size()),
			&opts);

		RR_CHECK(result == TG3_OK);

		// release
		tg3_model_free(&model);
		tg3_error_stack_free(&errors);
	}

	void BasicExecution()
	{
		rr::FileBatch batch(1024 * 1024); // 1 MB capacity
		std::filesystem::path test_file_path = "Assets/Models/kenney_cube_pets/animal-beaver.glb";

		RR_CHECK(batch.AddFile(test_file_path));
		batch.Clear();
		RR_CHECK(batch.GetUsedBytes() == 0);
	}

	void AddMultipleFiles()
	{
		rr::FileBatch batch(1024 * 1024); // 1 MB capacity
		std::filesystem::path test_file_path1 = "Assets/Models/kenney_cube_pets/animal-beaver.glb";
		std::filesystem::path test_file_path2 = "Assets/Models/kenney_cube_pets/animal-cat.glb";

		RR_CHECK(batch.AddFile(test_file_path1));
		RR_CHECK(batch.AddFile(test_file_path2));
		RR_CHECK(batch.AddFile(test_file_path1));
		RR_CHECK(batch.AddFile(test_file_path2));
	}

	void ExceedCapacity()
	{
		size_t const capacity_bytes = 1024 * 1024;
		rr::FileBatch batch(capacity_bytes);
		std::filesystem::path test_file_path = "Assets/Models/kenney_cube_pets/animal-beaver.glb";

		bool result = true;
		while (result)
		{
			result = batch.AddFile(test_file_path);
		}
		RR_CHECK(!result);
		RR_CHECK(batch.GetUsedBytes() < batch.GetArenaMaxSize());
	}

	void ParseWithTinyGLTFv3()
	{
		size_t const capacity_bytes = 1024 * 1024;
		rr::FileBatch batch(capacity_bytes);
		std::filesystem::path test_file_path1 = "Assets/Models/kenney_cube_pets/animal-beaver.glb";
		std::filesystem::path test_file_path2 = "Assets/Models/kenney_cube_pets/animal-cat.glb";
		batch.AddFile(test_file_path1);
		batch.AddFile(test_file_path2);

		batch.ForEach([&](FileBlob const& file_blob)
			{
				LoadGltfFromFileBlob(file_blob);
			});
	}

	void FileBatchTestJob(Job* self, FileBatch* batch)
	{
		batch->ForEach([&](FileBlob const& file_blob)
			{
				JobSystem& sys = *self->system;
				Job* child = sys.CreateJobAsChild<LoadGltfFromFileBlob>(self, file_blob);
				sys.RunJob(child);
			});
	}

	void PlayWithJobSystem()
	{
		Timer timer;

		JobSystem job_system;
		job_system.Initialize(8);

		size_t const capacity_bytes = 64 * 1024 * 1024;
		rr::FileBatch batch(capacity_bytes);
		std::filesystem::path test_file_path1 = "Assets/Models/kenney_cube_pets/animal-beaver.glb";
		std::filesystem::path test_file_path2 = "Assets/Models/kenney_cube_pets/animal-cat.glb";

		timer.Reset();

		size_t count = 0;
		bool result = true;
		while (result)
		{
			if (count % 2 == 0)
				result = batch.AddFile(test_file_path1);
			else
				result = batch.AddFile(test_file_path2);

			if (result)
				++count;
		}

		double file_read_ms = timer.ElapsedMilliseconds();
		LogOutput(LogLevel::Info, "FileBatch read {} files in {} ms", count, file_read_ms);

		timer.Reset();

		Job* root = job_system.CreateJob<FileBatchTestJob>(&batch);
		job_system.RunJob(root);
		job_system.WaitJob(root);
		job_system.Reset();

		RR_CHECK(job_system.GetTotalExecutedJobs() == count + 1);


		double parse_ms = timer.ElapsedMilliseconds();

		LogOutput(LogLevel::Info, "FileBatch parsed {} files in {} ms", count, parse_ms);
		job_system.Shutdown();
	}
}


namespace rr::test
{
	void TestFileBatch()
	{
		BasicExecution();
		AddMultipleFiles();
		ExceedCapacity();
		ParseWithTinyGLTFv3();
		PlayWithJobSystem();
	}

}
