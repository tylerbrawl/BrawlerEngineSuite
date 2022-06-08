module;
#include <vector>
#include <assimp/scene.h>

export module Brawler.StaticMeshResolver;
import Brawler.StaticVertexBuffer;
import Brawler.IndexBuffer;
import Brawler.MeshResolverBase;
import Brawler.ImportedMesh;
import Brawler.NormalBoundingCone;

export namespace Brawler
{
	class StaticMeshResolver final : public MeshResolverBase<StaticMeshResolver>
	{
	private:
		friend class MeshResolverBase<StaticMeshResolver>;

	public:
		explicit StaticMeshResolver(ImportedMesh&& mesh);

		StaticMeshResolver(const StaticMeshResolver& rhs) = delete;
		StaticMeshResolver& operator=(const StaticMeshResolver& rhs) = delete;

		StaticMeshResolver(StaticMeshResolver&& rhs) noexcept = default;
		StaticMeshResolver& operator=(StaticMeshResolver&& rhs) noexcept = default;

	private:
		void UpdateIMPL();
		bool IsReadyForSerializationIMPL() const;

	private:
		StaticVertexBuffer mVertexBuffer;
		IndexBuffer mIndexBuffer;
		std::vector<NormalBoundingCone> mNormalConeArr;
	};
}