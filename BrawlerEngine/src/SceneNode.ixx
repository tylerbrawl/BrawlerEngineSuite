module;
#include <vector>
#include <unordered_set>
#include <memory>
#include <atomic>
#include <cassert>

export module Brawler.SceneNode;
import Brawler.ComponentCollection;

export namespace Brawler
{
	class SceneGraph;
}

namespace Brawler
{
	template <typename T>
	static constexpr bool IS_CONST = std::is_const_v<std::remove_reference_t<T>>;
}

export namespace Brawler
{
	class SceneNode
	{
	private:
		friend class SceneGraph;

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
		std::size_t GetComponentCount() const;

		template <typename T>
		T* GetComponent();

		template <typename T>
			requires IS_CONST<T>
		const T* GetComponent() const;

		template <typename T>
		std::vector<T*> GetComponents();

		template <typename T>
			requires IS_CONST<T>
		std::vector<const T*> GetComponents() const;

		template <typename NodeType, typename... Args>
			requires std::derived_from<NodeType, SceneNode>
		void CreateChildSceneNode(Args&&... args);

		void RemoveChildSceneNode(SceneNode& childNode);

		SceneGraph& GetSceneGraph();
		const SceneGraph& GetSceneGraph() const;

		bool HasParentSceneNode() const;

		SceneNode& GetParentSceneNode();
		const SceneNode& GetParentSceneNode() const;

	private:
		void ExecuteSceneGraphUpdates();
		void ExecutePendingChildAdditions();
		void ExecutePendingChildRemovals();

		void SetSceneGraph(SceneGraph& sceneGraph);
		void SetParentNode(SceneNode& parentNode);

		void UpdateIMPL(const float dt);

	private:
		ComponentCollection mComponentCollection;
		SceneGraph* mSceneGraph;
		SceneNode* mParentNodePtr;

		/// <summary>
		/// These are the currently active child SceneNodes.
		/// </summary>
		std::vector<std::unique_ptr<SceneNode>> mChildNodePtrArr;

		/// <summary>
		/// These are the SceneNodes which are pending addition to the SceneGraph as
		/// a child of this SceneNode.
		/// </summary>
		std::vector<std::unique_ptr<SceneNode>> mPendingChildAdditionsArr;

		/// <summary>
		/// These are pointers to SceneNodes which are pending removal from the
		/// SceneGraph.
		/// </summary>
		std::unordered_set<SceneNode*> mPendingChildRemovals;
	};
}

// --------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	std::size_t SceneNode::GetComponentCount() const
	{
		return mComponentCollection.GetComponentCount<T>();
	}

	template <typename T>
	T* SceneNode::GetComponent()
	{
		return mComponentCollection.GetComponent<T>();
	}

	template <typename T>
		requires IS_CONST<T>
	const T* SceneNode::GetComponent() const
	{
		return mComponentCollection.GetComponent<T>();
	}

	template <typename T>
	std::vector<T*> SceneNode::GetComponents()
	{
		return mComponentCollection.GetComponents<T>();
	}

	template <typename T>
		requires IS_CONST<T>
	std::vector<const T*> SceneNode::GetComponents() const
	{
		return mComponentCollection.GetComponents<T>();
	}

	template <typename NodeType, typename... Args>
		requires std::derived_from<NodeType, SceneNode>
	void SceneNode::CreateChildSceneNode(Args&&... args)
	{
		using DecayedNodeT = std::decay_t<NodeType>;

		std::unique_ptr<SceneNode> childNodePtr{ std::make_unique<DecayedNodeT>(std::forward<Args>(args)...) };
		childNodePtr->SetParentNode(*this);

		assert(mSceneGraph != nullptr);
		childNodePtr->SetSceneGraph(*mSceneGraph);

		mPendingChildAdditionsArr.push_back(std::move(childNodePtr));
	}
}