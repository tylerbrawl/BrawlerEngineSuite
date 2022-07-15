module;
#include <cassert>
#include <map>
#include <set>
#include <memory>  // For some reason, we have to manually include this header here, even though it appears in ComponentCollection.ixx.
				   // Is this a compiler error?

module Brawler.ComponentCollection;

namespace Brawler
{
	ComponentCollection::ComponentCollection(SceneNode& owningNode) :
		mComponentMap(),
		mComponentsPendingRemoval(),
#ifdef _DEBUG
		mIsUpdating(false),
#endif
		mOwningNode(&owningNode)
	{}

	void ComponentCollection::Update(const float dt)
	{
		// First, remove all of the components which were pending removal from the last
		// update.
		ExecutePendingRemovals();

		// Now, update all of the components.
#ifdef _DEBUG
		mIsUpdating = true;
#endif // _DEBUG
		
		ExecuteComponentCommand([dt] (I_Component& component)
		{
			component.Update(dt);
		});

#ifdef _DEBUG
		mIsUpdating = false;
#endif // _DEBUG
	}

	void ComponentCollection::ExecutePendingRemovals()
	{
		mComponentsPendingRemoval.clear();
	}
}