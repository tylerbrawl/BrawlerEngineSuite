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
		AssimpMeshBuilder() = default;

		AssimpMeshBuilder(const AssimpMeshBuilder& rhs) = delete;
		AssimpMeshBuilder& operator=(const AssimpMeshBuilder& rhs) = delete;

		AssimpMeshBuilder(AssimpMeshBuilder&& rhs) noexcept = delete;
		AssimpMeshBuilder& operator=(AssimpMeshBuilder&& rhs) noexcept = delete;

		void InitializeMeshData(const aiMesh& mesh);
		void SetMaterialDefinitionHandle(MaterialDefinitionHandle&& hMaterial);

		Mesh CreateMesh();

	private:
		void InitializeVertexBufferData(const aiMesh& mesh);
		void InitializeIndexBufferData(const aiMesh& mesh);
		void InitializeAABBData(const aiMesh& mesh);

	private:
		MeshBuilder mBuilder;
	};
}