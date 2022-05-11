module;
#include <assimp/scene.h>

export module Brawler.StaticMeshResolver;
import Brawler.StaticVertexBuffer;
import Brawler.IndexBuffer;
import Brawler.MeshResolverBase;

export namespace Brawler
{
	class StaticMeshResolver final : public MeshResolverBase
	{
	public:
		explicit StaticMeshResolver(ImportedMesh&& mesh);

		StaticMeshResolver(const StaticMeshResolver& rhs) = delete;
		StaticMeshResolver& operator=(const StaticMeshResolver& rhs) = delete;

		StaticMeshResolver(StaticMeshResolver&& rhs) noexcept = default;
		StaticMeshResolver& operator=(StaticMeshResolver&& rhs) noexcept = default;

	protected:
		void UpdateIMPL();
		bool IsReadyForSerializationIMPL() const;

	private:
		StaticVertexBuffer mVertexBuffer;
		IndexBuffer mIndexBuffer;
	};
}