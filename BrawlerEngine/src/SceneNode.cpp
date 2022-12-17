module;
#include <vector>
#include <set>
#include <memory>
#include <atomic>
#include <cassert>

module Brawler.SceneNode;
import Brawler.JobSystem;
import Util.Engine;
import Util.Coroutine;

namespace Brawler
{
	void SceneNodeManager::UpdateSceneNodeIMPL(SceneNode& sceneNode, const float dt)
	{
		sceneNode.ExecuteSceneGraphUpdates();
		sceneNode.UpdateIMPL(dt);
	}
}

namespace Brawler
{
	SceneNode::SceneNode() :
		mComponentCollection(*this),
		mParentNodePtr(nullptr),
		mChildNodePtrArr(),
		mPendingChildAdditionsArr(),
		mPendingChildRemovals()
	{}
	
	void SceneNode::Update(const float dt)
	{}

	void SceneNode::LateUpdate(const float dt)
	{}

	void SceneNode::RemoveChildSceneNode(SceneNode& childNode)
	{
		for (auto& childNodePtr : mChildNodePtrArr)
		{
			if (childNodePtr.get() == &childNode)
			{
				mPendingChildRemovals.insert(childNodePtr.get());
				return;
			}
		}
	}

	bool SceneNode::HasParentSceneNode() const
	{
		return (mParentNodePtr != nullptr);
	}

	SceneNode& SceneNode::GetParentSceneNode()
	{
		assert(mParentNodePtr != nullptr);
		return *mParentNodePtr;
	}

	const SceneNode& SceneNode::GetParentSceneNode() const
	{
		assert(mParentNodePtr != nullptr);
		return *mParentNodePtr;
	}

	void SceneNode::ExecuteSceneGraphUpdates()
	{
		// First, execute the child node removals. That way, we have less
		// SceneNodes to iterate through.
		ExecutePendingChildRemovals();
		ExecutePendingChildAdditions();

		// Create a CPU job for each child node to do the same thing.
		//
		// TODO: Is it more efficient to do this single-threaded?
		Brawler::JobGroup childUpdateGroup{};
		childUpdateGroup.Reserve(mChildNodePtrArr.size());

		for (const auto& childNode : mChildNodePtrArr)
		{
			SceneNode* const childNodePtr = childNode.get();
			childUpdateGroup.AddJob([childNodePtr] ()
			{
				childNodePtr->ExecuteSceneGraphUpdates();
			});
		}

		childUpdateGroup.ExecuteJobs();
	}

	void SceneNode::ExecutePendingChildAdditions()
	{
		for (auto&& pendingChild : mPendingChildAdditionsArr)
			mChildNodePtrArr.push_back(std::move(pendingChild));

		mPendingChildAdditionsArr.clear();
	}

	void SceneNode::ExecutePendingChildRemovals()
	{
		std::erase_if(mChildNodePtrArr, [this] (const std::unique_ptr<SceneNode>& childNodePtr)
		{
			return mPendingChildRemovals.contains(childNodePtr.get());
		});

		mPendingChildRemovals.clear();
	}

	void SceneNode::SetParentNode(SceneNode& parentNode)
	{
		mParentNodePtr = &parentNode;
	}

	void SceneNode::UpdateIMPL(const float dt)
	{
		// First, update this SceneNode.
		Update(dt);
		mComponentCollection.Update(dt);
		LateUpdate(dt);

		// Create a separate CPU job for updating every child SceneNode.
		Brawler::JobGroup childUpdateGroup{};
		childUpdateGroup.Reserve(mChildNodePtrArr.size());

		for (const auto& childNode : mChildNodePtrArr)
		{
			SceneNode* const childNodePtr = childNode.get();
			childUpdateGroup.AddJob([childNodePtr, dt] ()
			{
				childNodePtr->UpdateIMPL(dt);
			});
		}

		childUpdateGroup.ExecuteJobs();
	}
}