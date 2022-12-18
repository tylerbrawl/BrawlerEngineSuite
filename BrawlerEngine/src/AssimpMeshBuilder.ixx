module;
#include <assimp/mesh.h>

export module Brawler.AssimpSceneLoader:AssimpMeshBuilder;
import Brawler.MeshBuilder;
import Brawler.MaterialDefinitionHandle;

export namespace Brawler
{
	class AssimpMeshBuilder
	{
	public:
		explicit AssimpMeshBuilder(const aiMesh& mesh);

		AssimpMeshBuilder(const AssimpMeshBuilder& rhs) = delete;
		AssimpMeshBuilder& operator=(const AssimpMeshBuilder& rhs) = delete;

		AssimpMeshBuilder(AssimpMeshBuilder&& rhs) noexcept = delete;
		AssimpMeshBuilder& operator=(AssimpMeshBuilder&& rhs) noexcept = delete;

		void InitializeMeshData();
		void SetMaterialDefinitionHandle(MaterialDefinitionHandle&& hMaterial);

		Mesh CreateMesh();

	private:
		void InitializeVertexBufferData();
		void InitializeIndexBufferData();
		void InitializeAABBData();

	private:
		MeshBuilder mBuilder;
		const aiMesh* mMeshPtr;
	};
}