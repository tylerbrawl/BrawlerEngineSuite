module;
#include <cassert>

module Brawler.SceneGraph;

namespace Brawler
{
	SceneGraph::SceneGraph() :
		mRootNode(),
		mIsUpdating(false)
	{
		mRootNode.SetSceneGraph(*this);
	}

	void SceneGraph::RemoveSceneNode(SceneNode& sceneNode)
	{
		assert(&mRootNode != &sceneNode && "ERROR: An attempt was made to remove the root SceneNode from a SceneGraph!");

		// Since we aren't dealing with the root SceneNode, we know that it has a
		// parent SceneNode which owns the SceneGraphEdge which owns it. So, we
		// tell the parent SceneNode to eventually remove this SceneNode.
		SceneNode& parentNode = sceneNode.GetOwningSceneGraphEdge().GetParentSceneNode();
		parentNode.RemoveChildSceneNode(sceneNode);
	}

	void SceneGraph::Update(const float dt)
	{
		mIsUpdating = true;
		
		mRootNode.UpdateIMPL(dt);

		mIsUpdating = false;
	}

	bool SceneGraph::IsUpdating() const
	{
		return mIsUpdating;
	}
}