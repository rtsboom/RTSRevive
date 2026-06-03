#pragma once
#include <string>
#include <memory>
#include "AssetBase.h"


namespace rr
{
	std::unique_ptr<AssetBase> ImportGLTF(std::string const& filename);
}
