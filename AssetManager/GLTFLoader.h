#pragma once
#include "StagingModel.h"

#include <string_view>

namespace rr::ModelLoader
{
	StagingModel LoadFromGLTF(std::string_view path);
}
