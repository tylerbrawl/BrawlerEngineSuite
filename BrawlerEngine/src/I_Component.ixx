module;

export module Brawler.I_Component;

namespace Brawler
{
	class ComponentCollection;
	class SceneNode;
}

export namespace Brawler
{
	class I_Component
	{
	private:
		friend class ComponentCollection;

	protected:
		explicit I_Component(SceneNode& owningNode);

	public:
		virtual ~I_Component() = default;

		I_Component(const I_Component& rhs) = delete;
		I_Component& operator=(const I_Component& rhs) = delete;

		I_Component(I_Component&& rhs) noexcept = default;
		I_Component& operator=(I_Component&& rhs) noexcept = default;

		virtual void Update(const float dt);

		SceneNode& GetSceneNode();
		const SceneNode& GetSceneNode() const;

	private:
		SceneNode* const mOwningNode;
	};
}