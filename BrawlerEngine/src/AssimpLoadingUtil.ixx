module;
#include <assimp/scene.h>

export module Brawler.AssimpSceneLoader:AssimpLoadingUtil;
import Brawler.Math.MathTypes;
import Brawler.SceneNode;

export namespace Util
{
	namespace AssimpLoading
	{
		Brawler::Math::Float4x4 CollapseWorldMatrixForAssimpNode(const aiNode& node);
		void TransformSceneNode(SceneNode& sceneNode, const aiNode& assimpNode);
	}
}