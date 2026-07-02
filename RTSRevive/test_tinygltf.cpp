#include "pch.h"
#include <TinyGLTFv3/tiny_gltf_v3.h>

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>

namespace rr::test
{
	bool RunTinyGltfTests()
	{
		std::string base_dir{ "Assets/Models/kenney_cube_pets/" };
		std::filesystem::path path{ "Assets/Models/kenney_cube_pets/animal-beaver.glb" };
		bool exist = std::filesystem::exists(path);
		if (!exist)
			return false;

		size_t file_size = std::filesystem::file_size(path);
		std::vector<uint8_t> file_bytes(file_size);

		std::ifstream file(path, std::ios::binary);
		file.read(reinterpret_cast<char*>(file_bytes.data()), file_size);

		if (!file)
			return false;

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
			file_bytes.data(),
			file_bytes.size(),
			base_dir.data(),
			static_cast<uint32_t>(base_dir.size()),
			&opts);

		if (result != TG3_OK)
			return false;

		return true;
	}
}