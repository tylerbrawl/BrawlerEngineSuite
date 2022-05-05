module;
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

export module Brawler.SceneLoader;
import Brawler.ModelResolver;

export namespace Brawler
{
	class SceneLoader
	{
	public:
		explicit SceneLoader(const std::wstring_view inputModelFilePath);

		SceneLoader(const SceneLoader& rhs) = delete;
		SceneLoader& operator=(const SceneLoader& rhs) = delete;

		SceneLoader(SceneLoader&& rhs) noexcept = default;
		SceneLoader& operator=(SceneLoader&& rhs) noexcept = default;

		void ProcessScene();

		const aiScene& GetScene() const;

	private:
		Assimp::Importer mAssimpImporter;
		const aiScene* mAiScene;
		ModelResolver mModelResolver;
	};
}