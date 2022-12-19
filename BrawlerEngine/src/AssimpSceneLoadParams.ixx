module;
#include <filesystem>
#include <assimp/scene.h>

export module Brawler.AssimpSceneLoader:AssimpSceneLoadParams;

export namespace Brawler
{
	struct AssimpSceneLoadParams
	{
		const aiScene& AssimpScene;
		const std::filesystem::path& SceneFilePath;
	};
}