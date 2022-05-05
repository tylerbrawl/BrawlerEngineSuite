module;

module Brawler.I_Component;

namespace Brawler
{
	I_Component::I_Component(SceneNode& owningNode) :
		mOwningNode(&owningNode)
	{}

	void I_Component::Update(const float dt)
	{}

	SceneNode& I_Component::GetSceneNode()
	{
		return *mOwningNode;
	}

	const SceneNode& I_Component::GetSceneNode() const
	{
		return *mOwningNode;
	}
}