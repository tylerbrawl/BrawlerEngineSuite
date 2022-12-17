module;
#include <concepts>
#include <memory>

export module Brawler.SceneGraph;
import Brawler.SceneNode;

export namespace Brawler
{
	class SceneGraph
	{
	public:
		SceneGraph();

		SceneGraph(const SceneGraph& rhs) = delete;
		SceneGraph& operator=(const SceneGraph& rhs) = delete;

		SceneGraph(SceneGraph&& rhs) noexcept = default;
		SceneGraph& operator=(SceneGraph&& rhs) noexcept = default;

		/// <summary>
		/// Creates a SceneNode of type NodeType and adds it as a child node of the singular
		/// root node of this SceneGraph instance.
		/// </summary>
		/// <typeparam name="NodeType">
		/// - The type of SceneNode to add. NodeType must derive from SceneNode.
		/// </typeparam>
		/// <typeparam name="...Args">
		/// - The types of the arguments specified by args which are to be forwarded to a
		/// constructor of NodeType.
		/// </typeparam>
		/// <param name="...args">
		/// - The arguments which are to be forwarded to a constructor of NodeType.
		/// </param>
		/// <returns>
		/// The function returns a reference to the created child SceneNode.
		/// </returns>
		template <typename NodeType, typename... Args>
			requires std::derived_from<NodeType, SceneNode>
		NodeType& CreateRootLevelSceneNode(Args&&... args);

		/// <summary>
		/// Adds the SceneNode instance of type NodeType as a child node of the singlular root
		/// node of this SceneGraph instance.
		/// </summary>
		/// <typeparam name="NodeType">
		/// - The type of SceneNode to add. NodeType must derive from SceneNode.
		/// </typeparam>
		/// <param name="sceneNodePtr">
		/// - The std::unique_ptr&lt;NodeType&gt; instance which represents the SceneNode which
		///   is to be added as a child node of the singular root node of this SceneGraph
		///   instance.
		/// </param>
		template <typename NodeType>
			requires std::derived_from<NodeType, SceneNode>
		void AddRootLevelSceneNode(std::unique_ptr<NodeType>&& sceneNodePtr);

		/// <summary>
		/// Marks the SceneNode specified by sceneNode for removal. It will be completely removed
		/// before the next SceneGraph update.
		/// </summary>
		/// <param name="sceneNode">
		/// - The SceneNode which is to be removed (eventually).
		/// </param>
		void RemoveSceneNode(SceneNode& sceneNode);

		/// <summary>
		/// Updates all of the SceneNodes and their components in the SceneGraph instance. Updates
		/// are done in a multi-threaded fashion whereby each SceneNode, along with all of its
		/// components, are guaranteed to be updated on the same thread. It is also guaranteed that
		/// a SceneNode is updated before any of its components are, and that child nodes are updated
		/// only after these components are updated.
		/// 
		/// However, outside of the guarantees listed above, there are no other guarantees about the
		/// order in which SceneNodes are updated or which thread they are updated on.
		/// </summary>
		/// <param name="dt">
		/// - The time (in seconds) which is used as the timestep for SceneNode updates.
		/// </param>
		void Update(const float dt);

		/// <summary>
		/// Checks if the SceneGraph is currently executing an update.
		/// </summary>
		/// <returns>
		/// The function returns true if the SceneGraph is currently executing an update and false
		/// otherwise.
		/// </returns>
		bool IsUpdating() const;

	private:
		SceneNode mRootNode;
		bool mIsUpdating;
	};
}

// -------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename NodeType, typename... Args>
		requires std::derived_from<NodeType, SceneNode>
	NodeType& SceneGraph::CreateRootLevelSceneNode(Args&&... args)
	{
		return mRootNode.CreateChildSceneNode<NodeType, Args...>(std::forward<Args>(args)...);
	}

	template <typename NodeType>
		requires std::derived_from<NodeType, SceneNode>
	void SceneGraph::AddRootLevelSceneNode(std::unique_ptr<SceneNode>&& sceneNodePtr)
	{
		mRootNode.AddChildSceneNode(std::move(sceneNodePtr));
	}
}