module;
#include <filesystem>
#include <assimp/scene.h>

export module Brawler.LODScene;

export namespace Brawler
{
	class LODScene
	{
	public:
		LODScene(const aiScene& scene, const std::uint32_t lodLevel);

		LODScene(const LODScene& rhs) = default;
		LODScene& operator=(const LODScene& rhs) = default;

		LODScene(LODScene&& rhs) noexcept = default;
		LODScene& operator=(LODScene&& rhs) noexcept = default;

		const aiScene& GetScene() const;
		std::uint32_t GetLODLevel() const;

		const std::filesystem::path& GetInputMeshFilePath() const;

	private:
		const aiScene* mAIScenePtr;
		std::uint32_t mLODLevel;
	};
}