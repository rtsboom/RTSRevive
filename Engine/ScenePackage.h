#pragma once
#include <Engine/StableArena.h>
#include <Engine/Material.h>
#include <Engine/Scene.h>
#include <cstdint>
#include <string_view>
namespace rr
{
	struct Mesh;
	struct Primitive;
	struct ScenePackage
	{
		static constexpr size_t kArenaSize = 32 * 1024 * 1024; // 32MB

		ScenePackage(ScenePackage const&) = delete;
		ScenePackage& operator=(ScenePackage const&) = delete;
		ScenePackage(ScenePackage&&) = default;
		ScenePackage& operator=(ScenePackage&&) = default;

		StableArena arena_{ kArenaSize };
		Scene* scene{ nullptr };

		Mesh* meshes{ nullptr };
		int   mesh_count{ 0 };

		Primitive* primitives{ nullptr };
		int		   primitive_count{ 0 };

		Sampler* samplers{ nullptr };
		int      sampler_count{ 0 };

		Texture* textures{ nullptr };
		int      texture_count{ 0 };

		std::string_view base_dir{};
		Image* images{ nullptr };
		int    image_count{ 0 };
	};

}