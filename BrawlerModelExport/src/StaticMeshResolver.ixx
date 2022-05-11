module;
#include <assimp/scene.h>

export module Brawler.StaticMeshResolver;
import Brawler.StaticVertexBuffer;
import Brawler.IndexBuffer;

export namespace Brawler
{
	class StaticMeshResolver
	{
	public:
		explicit StaticMeshResolver(const aiMesh& mesh);

		StaticMeshResolver(const StaticMeshResolver& rhs) = delete;
		StaticMeshResolver& operator=(const StaticMeshResolver& rhs) = delete;

		StaticMeshResolver(StaticMeshResolver&& rhs) noexcept = default;
		StaticMeshResolver& operator=(StaticMeshResolver&& rhs) noexcept = default;

	private:
		StaticVertexBuffer mVertexBuffer;
		IndexBuffer mIndexBuffer;
	};
}