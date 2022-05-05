module;
#include <assimp/scene.h>

module Brawler.StaticMesh;

namespace Brawler
{
	StaticMesh::StaticMesh(const aiMesh& mesh) :
		mVertexBuffer(mesh),
		mIndexBuffer(mesh)
	{}
}