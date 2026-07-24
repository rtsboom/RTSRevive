#pragma once
#include <string_view>
#include <cstdint>
namespace rr
{
	struct Image
	{
		std::string_view uri;
	};

	struct Material
	{
		int base_color_texture{ -1 };
		int base_color_sampler{ -1 };
		int metallic_roughness_texture{ -1 };
		int metallic_roughness_sampler{ -1 };
		int normal_texture{ -1 };
		int normal_sampler{ -1 };

		float base_color_factor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		float metallic_factor{ 0.0f };
		float roughness_factor{ 1.0f };
	};
}