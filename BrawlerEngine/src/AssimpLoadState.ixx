module;
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

export module Brawler.AssimpLoadState;
import Brawler.I_ApplicationState;

export namespace Brawler
{
	class AssimpLoadState final : public I_ApplicationState
	{
	public:
		explicit AssimpLoadState(std::filesystem::path modelFilePath);

		AssimpLoadState(const AssimpLoadState& rhs) = delete;
		AssimpLoadState& operator=(const AssimpLoadState& rhs) = delete;

		AssimpLoadState(AssimpLoadState&& rhs) noexcept = default;
		AssimpLoadState& operator=(AssimpLoadState&& rhs) noexcept = default;

		void Update(const float dt) override;

	private:
		void ImportModelFile();

	private:
		Assimp::Importer mImporter;
		const aiScene* mScenePtr;
		std::filesystem::path mModelFilePath;
	};
}