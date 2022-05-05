module;
#include <memory>

module Brawler.SceneGraphEdge;
import Brawler.SceneNode;

namespace Brawler
{
	SceneGraphEdge::SceneGraphEdge(SceneNode& parentNode, std::unique_ptr<SceneNode>&& childNode) :
		mParentNode(&parentNode),
		mChildNode(std::move(childNode))
	{
		mChildNode->SetSceneGraph(parentNode.GetSceneGraph());
	}

	void SceneGraphEdge::Update(const float dt)
	{}

	void SceneGraphEdge::ExecuteSceneGraphUpdates()
	{
		mChildNode->ExecuteSceneGraphUpdates();
	}

	void SceneGraphEdge::InitializeChildNodeUpdateTickCounter()
	{
		mChildNode->InitializeUpdateTickCounter();
	}

	void SceneGraphEdge::UpdateIMPL(const float dt)
	{
		// First, update the SceneGraphEdge. This allows for data transfer between
		// parent and child.
		Update(dt);

		mChildNode->UpdateIMPL(dt);
	}

	SceneNode& SceneGraphEdge::GetParentSceneNode()
	{
		return *mParentNode;
	}

	const SceneNode& SceneGraphEdge::GetParentSceneNode() const
	{
		return *mParentNode;
	}

	SceneNode& SceneGraphEdge::GetChildSceneNode()
	{
		return *mChildNode;
	}

	const SceneNode& SceneGraphEdge::GetChildSceneNode() const
	{
		return *mChildNode;
	}
}