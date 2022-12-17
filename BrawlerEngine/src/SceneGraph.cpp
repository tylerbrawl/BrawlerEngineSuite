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
		assert(sceneNode.HasParentSceneNode() && "ERROR: An attempt was made to remove the root SceneNode from a SceneGraph!");
		assert(&mRootNode != &sceneNode);

		// Since we aren't dealing with the root SceneNode, we know that sceneNode
		// has a parent SceneNode which owns it. So, we tell the parent SceneNode to 
		// eventually remove this SceneNode.
		SceneNode& parentNode = sceneNode.GetParentSceneNode();
		parentNode.RemoveChildSceneNode(sceneNode);
	}

	void SceneGraph::Update(const float dt)
	{
		mIsUpdating = true;
		
		SceneNodeManager::UpdateSceneNodeIMPL(mRootNode, dt);

		mIsUpdating = false;
	}

	bool SceneGraph::IsUpdating() const
	{
		return mIsUpdating;
	}
}