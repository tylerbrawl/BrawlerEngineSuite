module;
#include <assimp/scene.h>

module Brawler.StaticMeshResolver;

namespace Brawler
{
	StaticMeshResolver::StaticMeshResolver(const aiMesh& mesh) :
		mVertexBuffer(mesh),
		mIndexBuffer(mesh)
	{}
}