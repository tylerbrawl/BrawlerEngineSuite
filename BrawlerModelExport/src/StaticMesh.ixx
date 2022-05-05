module;
#include <assimp/scene.h>

export module Brawler.StaticMesh;
import Brawler.StaticVertexBuffer;
import Brawler.IndexBuffer;

export namespace Brawler
{
	class StaticMesh
	{
	public:
		explicit StaticMesh(const aiMesh& mesh);

		StaticMesh(const StaticMesh& rhs) = delete;
		StaticMesh& operator=(const StaticMesh& rhs) = delete;

		StaticMesh(StaticMesh&& rhs) noexcept = default;
		StaticMesh& operator=(StaticMesh&& rhs) noexcept = default;

	private:
		StaticVertexBuffer mVertexBuffer;
		IndexBuffer mIndexBuffer;
	};
}