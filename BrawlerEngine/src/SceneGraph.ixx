module;
#include <concepts>

export module Brawler.SceneGraph;
import Brawler.SceneNode;
import Brawler.SceneGraphEdge;

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
		/// root node of the SceneGraph.
		/// </summary>
		/// <typeparam name="NodeType">
		/// - The type of SceneNode to add. NodeType must derive from SceneNode.
		/// </typeparam>
		/// <typeparam name="EdgeType">
		/// - The type of SceneGraphEdge which will be used to connect the root node of the
		/// SceneGraph to the newly created child node. By default, this is the same type
		/// as SceneGraphEdge.
		/// </typeparam>
		/// <typeparam name="...Args">
		/// - The types of the arguments specified by args which are to be forwarded to a
		/// constructor of NodeType.
		/// </typeparam>
		/// <param name="...args">
		/// - The arguments which are to be forwarded to a constructor of NodeType.
		/// </param>
		template <typename NodeType, typename EdgeType = SceneGraphEdge, typename... Args>
			requires std::derived_from<NodeType, SceneNode> && std::derived_from<EdgeType, SceneGraphEdge>
		void AddRootLevelSceneNode(Args&&... args);

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
		/// - The time (in milliseconds) which is used as the timestep for SceneNode updates.
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
	template <typename NodeType, typename EdgeType, typename... Args>
		requires std::derived_from<NodeType, SceneNode>&& std::derived_from<EdgeType, SceneGraphEdge>
	void SceneGraph::AddRootLevelSceneNode(Args&&... args)
	{
		mRootNode.CreateChildSceneNode<NodeType, EdgeType, Args...>(std::forward<Args>(args)...);
	}
}