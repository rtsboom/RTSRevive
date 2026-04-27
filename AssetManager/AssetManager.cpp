#include "pch.h"
#include "AssetManager.h"
#include "StagingModel.h"
#include "GLTFLoader.h"
namespace rr
{
	ModelHandle AssetManager::LoadModel(std::string_view path)
	{
		StagingModel staging = {};
		if (path.ends_with(".glb") || path.ends_with(".gltf"))
		{
			staging = ModelLoader::LoadFromGLTF(path);
		}

		return ModelHandle();
	}
}