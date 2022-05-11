module;
#include <assimp/scene.h>

export module Brawler.ImportedMesh;

export namespace Brawler
{
	class ImportedMesh
	{
	public:
		ImportedMesh() = default;
		ImportedMesh(const aiMesh& mesh, const aiScene& owningScene);

		// The ImportedMesh doesn't actually own the aiMesh* and aiScene*, so
		// allowing default copy and move is fine.

		ImportedMesh(const ImportedMesh& rhs) = default;
		ImportedMesh& operator=(const ImportedMesh& rhs) = default;
		
		ImportedMesh(ImportedMesh&& rhs) noexcept = default;
		ImportedMesh& operator=(ImportedMesh&& rhs) noexcept = default;

		const aiMesh& GetMesh() const;
		const aiScene& GetOwningScene() const;

		/// <summary>
		/// Retrieves the unique aiMaterial instance of the aiMesh represented by this
		/// ImportedMesh instance. 
		/// 
		/// Getting the aiMaterial directly from only the aiMesh instance is not possible; 
		/// it also requires accessing the aiScene instance which references the aiMesh instance.
		/// This function provides a convenient method for accessing the material.
		/// </summary>
		/// <returns></returns>
		const aiMaterial& GetMeshMaterial() const;

	private:
		const aiMesh* mAIMeshPtr;
		const aiScene* mAIScenePtr;
	};
}