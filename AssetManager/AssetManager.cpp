#include "pch.h"
#include "AssetManager.h"
#include "StagingModel.h"
#include "GLTFLoader.h"
#include "Model.h"
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
	Model AssetManager::CreateModel(std::string_view path)
	{
		Model model = {};

		return model;
	}
}