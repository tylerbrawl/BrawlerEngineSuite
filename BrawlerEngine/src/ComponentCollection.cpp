module;
#include <cassert>
#include <map>
#include <vector>
#include <set>
#include <memory>  // For some reason, we have to manually include this header here, even though it appears in ComponentCollection.ixx.
				   // Is this a compiler error?

module Brawler.ComponentCollection;

namespace Brawler
{
	ComponentCollection::ComponentCollection(SceneNode& owningNode) :
		mComponentMap(),
		mComponentsPendingAddition(),
		mComponentsPendingRemoval(),
		mComponentsMarkedForRemovalDuringUpdateArr(),
		mIsUpdating(false),
		mOwningNode(&owningNode)
	{}

	void ComponentCollection::Update(const float dt)
	{
		// First, remove all of the components which were pending removal from the last
		// update.
		ExecutePendingRemovals();

		// Now, update all of the components.
		mIsUpdating = true;

		ExecuteComponentCommand([dt] (I_Component& component)
		{
			component.Update(dt);
		});

		mIsUpdating = false;

		// If any requests were made to remove a component during the update, we fulfill them now.
		for (const auto& removalInfo : mComponentsMarkedForRemovalDuringUpdateArr)
		{
			assert(mComponentMap.contains(removalInfo.CompID));
			auto& currContainer{ mComponentMap.at(removalInfo.CompID) };

			for (auto itr = currContainer.begin(); itr != currContainer.end();)
			{
				if (static_cast<I_Component*>(itr->get()) == removalInfo.ComponentPtr)
				{
					(*itr)->OnComponentRemoval();
					mComponentsPendingRemoval.push_back(std::move(*itr));

					itr = currContainer.erase(itr);
				}
				else
					++itr;
			}
		}

		mComponentsMarkedForRemovalDuringUpdateArr.clear();

		// Similarly, we now fulfill any component creation requests which were made during the
		// update.
		for (auto& additionInfo : mComponentsPendingAddition)
			mComponentMap[additionInfo.CompID].push_back(std::move(additionInfo.ComponentPtr));

		mComponentsPendingAddition.clear();
	}

	void ComponentCollection::PrepareForSceneNodeDeletion()
	{
		assert(!mIsUpdating);

		for (auto&& [compID, compPtrArr] : mComponentMap)
		{
			for (auto&& compPtr : compPtrArr)
			{
				compPtr->OnComponentRemoval();
				mComponentsPendingRemoval.push_back(std::move(compPtr));
			}
		}

		mComponentsPendingRemoval.clear();
	}

	bool ComponentCollection::IsSceneNodeDeletionSafe()
	{
		assert(mComponentMap.empty());
		assert(!mIsUpdating);
		
		ExecutePendingRemovals();
		return mComponentsPendingRemoval.empty();
	}

	void ComponentCollection::ExecutePendingRemovals()
	{
		std::erase_if(mComponentsPendingRemoval, [] (const std::unique_ptr<I_Component>& componentPtr) { return componentPtr->IsSafeToDelete(); });
	}
}