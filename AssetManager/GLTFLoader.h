#pragma once
#include "StagingModel.h"
#include <string>
#include <memory>
#include "AssetBase.h"


namespace rr
{
	std::unique_ptr<AssetBase> ImportGLTF(std::string const& filename);
}

namespace rr::ModelLoader
{
	StagingModel LoadFromGLTF(std::string_view path);
}
