module;
#include <filesystem>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

export module Brawler.LODResolver;
import Brawler.MeshResolverCollection;

export namespace Brawler
{
	class LODResolver
	{
		// We disallow default constructors because Assimp::Importer instances are
		// expensive to construct.
		LODResolver() = delete;
		
		explicit LODResolver(const std::uint32_t lodLevel);

		LODResolver(const LODResolver& rhs) = delete;
		LODResolver& operator=(const LODResolver& rhs) = delete;

		LODResolver(LODResolver&& rhs) noexcept = default;
		LODResolver& operator=(LODResolver&& rhs) noexcept = default;

		void ImportScene(const std::filesystem::path& fbxFile);

		void Update();
		bool IsReadyForSerialization() const;

		const aiScene& GetScene() const;
		std::uint32_t GetLODLevel() const;

	private:
		void CreateAIScene(const std::filesystem::path& fbxFile);
		void CreateMeshResolvers();

	private:
		/// <summary>
		/// Although Assimp::Importer instances are expensive to construct, according
		/// to the documentation, they are not thread-safe, so we need a separate instance
		/// for each LODResolver instance.
		/// </summary>
		Assimp::Importer mImporter;

		const aiScene* mAIScenePtr;
		MeshResolverCollection mMeshResolverCollection;
		std::uint32_t mLODLevel;
	};
}