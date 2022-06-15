module;
#include <vector>
#include <assimp/scene.h>

export module Brawler.StaticMeshResolver;
import Brawler.StaticVertexBuffer;
import Brawler.IndexBuffer;
import Brawler.MeshResolverBase;
import Brawler.ImportedMesh;
import Brawler.SerializedStaticMeshData;

export namespace Brawler
{
	class StaticMeshResolver final : public MeshResolverBase<StaticMeshResolver>
	{
	public:
		using SerializedMeshData = SerializedStaticMeshData;

	public:
		explicit StaticMeshResolver(std::unique_ptr<ImportedMesh>&& meshPtr);

		StaticMeshResolver(const StaticMeshResolver& rhs) = delete;
		StaticMeshResolver& operator=(const StaticMeshResolver& rhs) = delete;

		StaticMeshResolver(StaticMeshResolver&& rhs) noexcept = default;
		StaticMeshResolver& operator=(StaticMeshResolver&& rhs) noexcept = default;

		void UpdateIMPL();
		bool IsReadyForSerializationIMPL() const;

		SerializedMeshData SerializeMeshDataIMPL() const;

	private:
		StaticVertexBuffer mVertexBuffer;
		IndexBuffer mIndexBuffer;
	};
}