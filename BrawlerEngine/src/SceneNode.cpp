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
import Brawler.SceneGraph;

namespace Brawler
{
	SceneNode::SceneNode() :
		mComponentCollection(*this),
		mSceneGraph(nullptr),
		mOwningEdge(nullptr),
		mChildNodeEdges(),
		mPendingChildAdditions(),
		mPendingChildRemovals(),
		mCurrUpdateTick(Util::Engine::GetCurrentUpdateTick())
	{}
	
	void SceneNode::Update(const float dt)
	{}

	void SceneNode::RemoveChildSceneNode(SceneNode& childNode)
	{
		for (auto& childEdge : mChildNodeEdges)
		{
			if (&(childEdge->GetChildSceneNode()) == &childNode)
			{
				mPendingChildRemovals.insert(childEdge.get());
				return;
			}
		}
	}

	SceneGraph& SceneNode::GetSceneGraph()
	{
		return *mSceneGraph;
	}

	const SceneGraph& SceneNode::GetSceneGraph() const
	{
		return *mSceneGraph;
	}

	bool SceneNode::IsUpdateFinished() const
	{
		return (mCurrUpdateTick != Util::Engine::GetCurrentUpdateTick());
	}

	void SceneNode::WaitForUpdate() const
	{
		assert(GetSceneGraph().IsUpdating() && "ERROR: SceneNode::WaitForUpdate() was called when the SceneGraph was *NOT* executing an update!");
		
		while (!IsUpdateFinished())
			Util::Coroutine::TryExecuteJob();
	}

	void SceneNode::ExecuteSceneGraphUpdates()
	{
		// First, execute the child node removals. That way, we have less
		// SceneGraphEdges to iterate through.
		ExecutePendingChildRemovals();
		ExecutePendingChildAdditions();

		// Create a CPU job for each child node to do the same thing.
		//
		// TODO: Is it more efficient to do this single-threaded?
		Brawler::JobGroup childUpdateGroup{};
		childUpdateGroup.Reserve(mChildNodeEdges.size());

		for (auto& childEdge : mChildNodeEdges)
		{
			SceneGraphEdge* const edgePtr = childEdge.get();
			childUpdateGroup.AddJob([edgePtr] ()
			{
				edgePtr->ExecuteSceneGraphUpdates();
			});
		}

		childUpdateGroup.ExecuteJobs();
	}

	void SceneNode::ExecutePendingChildAdditions()
	{
		for (auto&& pendingChild : mPendingChildAdditions)
		{
			// Now that this node is going to be receiving updates, we need to
			// initialize its update tick counter.
			pendingChild->InitializeChildNodeUpdateTickCounter();
			
			mChildNodeEdges.push_back(std::move(pendingChild));
		}

		mPendingChildAdditions.clear();
	}

	void SceneNode::ExecutePendingChildRemovals()
	{
		for (auto itr = mChildNodeEdges.begin(); itr != mChildNodeEdges.end();)
		{
			if (mPendingChildRemovals.contains(itr->get()))
				itr = mChildNodeEdges.erase(itr);
			else
				++itr;
		}

		mPendingChildRemovals.clear();
	}

	void SceneNode::InitializeUpdateTickCounter()
	{
		mCurrUpdateTick.store(Util::Engine::GetCurrentUpdateTick());
	}

	void SceneNode::SetSceneGraph(SceneGraph& sceneGraph)
	{
		mSceneGraph = &sceneGraph;
	}

	SceneGraphEdge& SceneNode::GetOwningSceneGraphEdge()
	{
		assert(mOwningEdge != nullptr && "ERROR: An attempt was made to get the owning SceneGraphEdge of a SceneNode which did not have one! (This should never be the case, unless the SceneNode is the root node of a SceneGraph.)");

		return *mOwningEdge;
	}

	const SceneGraphEdge& SceneNode::GetOwningSceneGraphEdge() const
	{
		assert(mOwningEdge != nullptr && "ERROR: An attempt was made to get the owning SceneGraphEdge of a SceneNode which did not have one! (This should never be the case, unless the SceneNode is the root node of a SceneGraph.)");

		return *mOwningEdge;
	}

	void SceneNode::SetOwningSceneGraphEdge(SceneGraphEdge& owningEdge)
	{
		mOwningEdge = &owningEdge;
	}

	void SceneNode::UpdateIMPL(const float dt)
	{
		// First, update this SceneNode.
		Update(dt);
		mComponentCollection.Update(dt);

		// Mark this SceneNode's update as having been completed.
		++mCurrUpdateTick;

		// Create a separate CPU job for updating every child SceneNode.
		Brawler::JobGroup childUpdateGroup{};
		childUpdateGroup.Reserve(mChildNodeEdges.size());

		for (auto& childNodeEdge : mChildNodeEdges)
		{
			SceneGraphEdge* const edgePtr = childNodeEdge.get();
			childUpdateGroup.AddJob([edgePtr, dt] ()
			{
				edgePtr->Update(dt);
			});
		}

		childUpdateGroup.ExecuteJobs();
	}
}