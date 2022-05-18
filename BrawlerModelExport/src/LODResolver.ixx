module;
#include <filesystem>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

export module Brawler.LODResolver;
import Brawler.MeshResolverCollection;
import Brawler.ModelTextureBuilderCollection;

export namespace Brawler
{
	class LODResolver
	{
	public:
		// We disallow default constructors because Assimp::Importer instances are
		// expensive to construct.
		LODResolver() = delete;
		
		explicit LODResolver(const std::uint32_t lodLevel);

		LODResolver(const LODResolver& rhs) = delete;
		LODResolver& operator=(const LODResolver& rhs) = delete;

		LODResolver(LODResolver&& rhs) noexcept = default;
		LODResolver& operator=(LODResolver&& rhs) noexcept = default;

		/// <summary>
		/// Begins the scene importing process, using the LOD value assigned to this LODResolver
		/// in its constructor.
		/// 
		/// This is a relatively long-running process, and it is done only on a single thread. To 
		/// improve efficiency, all of the LOD mesh files should have their scenes be imported
		/// concurrently. This is guaranteed to be thread safe.
		/// </summary>
		void ImportScene();

		ModelTextureBuilderCollection CreateModelTextureBuilders();

		void Update();
		bool IsReadyForSerialization() const;

		const aiScene& GetScene() const;
		std::uint32_t GetLODLevel() const;

	private:
		void CreateAIScene();
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