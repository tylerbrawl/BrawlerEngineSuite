module;
#include <cassert>

module Brawler.I_Component;

namespace Brawler
{
	void I_Component::Update(const float dt)
	{}

	SceneNode& I_Component::GetSceneNode()
	{
		assert(mOwningNode != nullptr);
		return *mOwningNode;
	}

	const SceneNode& I_Component::GetSceneNode() const
	{
		assert(mOwningNode != nullptr);
		return *mOwningNode;
	}

	void I_Component::SetSceneNode(SceneNode& owningNode)
	{
		mOwningNode = &owningNode;
	}

	void I_Component::OnComponentRemoval()
	{}

	bool I_Component::IsSafeToDelete() const
	{
		return true;
	}
}