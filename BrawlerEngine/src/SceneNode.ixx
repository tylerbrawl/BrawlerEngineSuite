module;
#include <vector>
#include <set>
#include <memory>
#include <atomic>

export module Brawler.SceneNode;
import Brawler.I_Component;
import Brawler.ComponentCollection;
import Brawler.SceneGraphEdge;

export namespace Brawler
{
	class SceneGraph;
}

namespace IMPL
{
	template <typename T>
	concept IsValidComponent = std::derived_from<T, Brawler::I_Component> && !std::is_pointer_v<T>;
}

export namespace Brawler
{
	class SceneNode
	{
	private:
		friend class SceneGraph;
		friend class SceneGraphEdge;

	protected:
		SceneNode();

	public:
		virtual ~SceneNode() = default;

		SceneNode(const SceneNode& rhs) = delete;
		SceneNode& operator=(const SceneNode& rhs) = delete;

		SceneNode(SceneNode&& rhs) noexcept = default;
		SceneNode& operator=(SceneNode&& rhs) noexcept = default;

		virtual void Update(const float dt);

		template <typename T>
			requires std::derived_from<T, I_Component>
		std::size_t GetComponentCount() const;

		template <typename T, typename DecayedT = std::decay_t<T>>
			requires IMPL::IsValidComponent<T>
		DecayedT* GetComponent();

		template <typename T, typename DecayedT = std::decay_t<T>>
			requires IMPL::IsValidComponent<T>
		const DecayedT* GetComponent() const;

		template <typename T, typename DecayedT = std::decay_t<T>>
			requires IMPL::IsValidComponent<T>
		std::vector<DecayedT*> GetComponents();

		template <typename T, typename DecayedT = std::decay_t<T>>
			requires IMPL::IsValidComponent<T>
		std::vector<const DecayedT*> GetComponents() const;

		template <typename NodeType, typename EdgeType = SceneGraphEdge, typename... Args>
			requires std::derived_from<NodeType, SceneNode> && std::derived_from<EdgeType, SceneGraphEdge>
		void CreateChildSceneNode(Args&&... args);

		void RemoveChildSceneNode(SceneNode& childNode);

		SceneGraph& GetSceneGraph();
		const SceneGraph& GetSceneGraph() const;

		/// <summary>
		/// Checks whether this particular SceneNode and all of its components have 
		/// finished updating for the current update tick. Note that this function 
		/// can return true even if child SceneNode instances have not finished 
		/// updating.
		/// </summary>
		/// <returns>
		/// The function returns true if this SceneNode instance, along with all of
		/// it components, have all finished updating for the current update tick 
		/// and false otherwise.
		/// </returns>
		bool IsUpdateFinished() const;

		/// <summary>
		/// Executes queued CPU jobs until this SceneNode and all of its components have
		/// finished updating. Use this to efficiently wait for a SceneNode instance to 
		/// finish updating before resuming execution.
		/// 
		/// NOTE: Beware circular dependencies between SceneNode update waits, for these
		/// can cause a livelock!
		/// </summary>
		void WaitForUpdate() const;

	private:
		void ExecuteSceneGraphUpdates();
		void ExecutePendingChildAdditions();
		void ExecutePendingChildRemovals();

		/// <summary>
		/// Resets the update tick counter of this SceneNode instance to the current update
		/// tick returned by Util::Engine::GetCurrentUpdateTick(). 
		/// 
		/// This is called when a SceneNode which was pending addition to the SceneGraph is 
		/// actually added to it.
		/// </summary>
		void InitializeUpdateTickCounter();

		void SetSceneGraph(SceneGraph& sceneGraph);
		
		SceneGraphEdge& GetOwningSceneGraphEdge();
		const SceneGraphEdge& GetOwningSceneGraphEdge() const;

		void SetOwningSceneGraphEdge(SceneGraphEdge& owningEdge);
		void UpdateIMPL(const float dt);

	private:
		ComponentCollection mComponentCollection;
		SceneGraph* mSceneGraph;
		SceneGraphEdge* mOwningEdge;

		/// <summary>
		/// These are the currently active child SceneNodes (or, to be more precise, their
		/// owning SceneGraphEdges).
		/// </summary>
		std::vector<std::unique_ptr<SceneGraphEdge>> mChildNodeEdges;

		/// <summary>
		/// These are the SceneGraphEdges which are pending addition to the SceneGraph as
		/// a child of this SceneNode.
		/// </summary>
		std::vector<std::unique_ptr<SceneGraphEdge>> mPendingChildAdditions;

		/// <summary>
		/// These are pointers to SceneGraphEdges which are pending removal from the
		/// SceneGraph. They should point to child edges of this SceneNode instance.
		/// </summary>
		std::set<SceneGraphEdge*> mPendingChildRemovals;

		std::atomic<std::uint64_t> mCurrUpdateTick;
	};
}

// --------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_Component>
	std::size_t SceneNode::GetComponentCount() const
	{
		return mComponentCollection.GetComponentCount<T>();
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	DecayedT* SceneNode::GetComponent()
	{
		return mComponentCollection.GetComponent<T>();
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	const DecayedT* SceneNode::GetComponent() const
	{
		return mComponentCollection.GetComponent<T>();
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	std::vector<DecayedT*> SceneNode::GetComponents()
	{
		return mComponentCollection.GetComponents<T>();
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	std::vector<const DecayedT*> SceneNode::GetComponents() const
	{
		return mComponentCollection.GetComponents<T>();
	}

	template <typename NodeType, typename EdgeType, typename... Args>
		requires std::derived_from<NodeType, SceneNode> && std::derived_from<EdgeType, SceneGraphEdge>
	void SceneNode::CreateChildSceneNode(Args&&... args)
	{
		using DecayedNodeT = std::decay_t<NodeType>;
		using DecayedEdgeT = std::decay_t<EdgeType>;

		std::unique_ptr<SceneNode> childNode{ std::make_unique<DecayedNodeT>(std::forward<Args>(args)...) };
		SceneNode* const childNodePtr = childNode.get();

		std::unique_ptr<SceneGraphEdge> graphEdge{ std::make_unique<DecayedEdgeT>(*this, std::move(childNode)) };
		childNode->SetOwningSceneGraphEdge(*(graphEdge.get()));

		mPendingChildAdditions.push_back(std::move(graphEdge));
	}
}