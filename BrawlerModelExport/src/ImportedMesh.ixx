module;
#include <cstdint>
#include <assimp/scene.h>

export module Brawler.ImportedMesh;
import Brawler.LODScene;

export namespace Brawler
{
	class ImportedMesh
	{
	public:
		ImportedMesh() = default;
		ImportedMesh(const aiMesh& mesh, const std::uint32_t meshIDForLOD, LODScene&& owningScene);

		// The ImportedMesh doesn't actually own the aiMesh* and aiScene*, so
		// allowing default copy and move is fine.

		ImportedMesh(const ImportedMesh& rhs) = default;
		ImportedMesh& operator=(const ImportedMesh& rhs) = default;
		
		ImportedMesh(ImportedMesh&& rhs) noexcept = default;
		ImportedMesh& operator=(ImportedMesh&& rhs) noexcept = default;

		const aiMesh& GetMesh() const;
		const aiScene& GetOwningScene() const;

		/// <summary>
		/// Retrieves the number which uniquely identifies this mesh within its LOD mesh.
		/// 
		/// *NOTE*: This value is *NOT* unique between LOD meshes!
		/// </summary>
		/// <returns>
		/// The function returns the number which uniquely identifies this mesh within its LOD
		/// mesh.
		/// </returns>
		std::uint32_t GetMeshIDForLOD() const;

		/// <summary>
		/// Retrieves the unique aiMaterial instance of the aiMesh represented by this
		/// ImportedMesh instance. 
		/// 
		/// Getting the aiMaterial directly from only the aiMesh instance is not possible; 
		/// it also requires accessing the aiScene instance which references the aiMesh instance.
		/// This function provides a convenient method for accessing the material.
		/// </summary>
		/// <returns>
		/// The function returns the unique aiMaterial instance of the aiMesh represented by this
		/// ImportedMesh instance.
		/// </returns>
		const aiMaterial& GetMeshMaterial() const;

		LODScene GetLODScene() const;

	private:
		const aiMesh* mAIMeshPtr;
		LODScene mOwningScene;
		std::uint32_t mMeshIDForLOD;
	};
}