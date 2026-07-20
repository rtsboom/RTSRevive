#pragma once
#include <string_view>
namespace rr
{
	struct Sampler
	{
		enum class Filter
		{
			Nearest,
			Linear,
			Nearest_Mipmap_Nearest,
			Linear_Mipmap_Nearest,
			Nearest_Mipmap_Linear,
			Linear_Mipmap_Linear,
		};
		enum class Wrap
		{
			Repeat,
			MirroredRepeat,
			ClampToEdge,
		};
		Filter min_filter{ Filter::Linear };
		Filter mag_filter{ Filter::Linear };
		Wrap wrap_u{ Wrap::Repeat };
		Wrap wrap_v{ Wrap::Repeat };
	};

	struct Image
	{
		std::string_view uri;
	};

	struct Texture
	{
		int image{ -1 };
		int sampler{ -1 };
	};

	struct Material
	{
		int base_color{ -1 };
		int metallic_roughness{ -1 };
		int normal{ -1 };

		float base_color_factor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		float metallic_factor{ 0.0f };
		float roughness_factor{ 1.0f };
	};

}