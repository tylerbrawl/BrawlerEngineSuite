module;
#include <memory>

export module Brawler.SceneGraphEdge;

export namespace Brawler
{
	class SceneNode;
}

export namespace Brawler
{
	class SceneGraphEdge
	{
	public:
		SceneGraphEdge(SceneNode& parentNode, std::unique_ptr<SceneNode>&& childNode);
		virtual ~SceneGraphEdge() = default;

		SceneGraphEdge(const SceneGraphEdge& rhs) = delete;
		SceneGraphEdge& operator=(const SceneGraphEdge& rhs) = delete;

		SceneGraphEdge(SceneGraphEdge&& rhs) noexcept = default;
		SceneGraphEdge& operator=(SceneGraphEdge&& rhs) noexcept = default;

		virtual void Update(const float dt);

		void ExecuteSceneGraphUpdates();
		void InitializeChildNodeUpdateTickCounter();

		void UpdateIMPL(const float dt);

		SceneNode& GetParentSceneNode();
		const SceneNode& GetParentSceneNode() const;

		SceneNode& GetChildSceneNode();
		const SceneNode& GetChildSceneNode() const;

	private:
		SceneNode* mParentNode;
		std::unique_ptr<SceneNode> mChildNode;
	};
}