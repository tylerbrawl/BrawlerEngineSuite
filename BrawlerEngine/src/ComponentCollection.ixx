module;
#include <vector>
#include <unordered_map>
#include <memory>
#include <cassert>
#include <set>

export module Brawler.ComponentCollection;
import Brawler.Functional;
import Brawler.I_Component;
import Brawler.ComponentID;
import Brawler.ComponentMap;

export namespace Brawler
{
	class SceneNode;
}

namespace IMPL
{
	template <typename T>
	concept IsValidComponent = std::derived_from<T, Brawler::I_Component> && !std::is_pointer_v<T>;
}

/*
Remarks:

* We disallow creating or removing I_Component instances while a ComponentCollection is being updated.
We really had two choices for handling this scenario:

	- Disallow it from happening in the first place.

	- Create a set of rules and conditions for what happens to I_Components which are added or removed
	  during a ComponentCollection update.

If we had chosen the latter option, we would have to answer questions like the following:

	- When should an I_Component be useable if it is created during an update? Should it be immediately
	  useable, or must we wait until after the update?
	
	- Does an I_Component which was added during an update still get updated during that update, or
	  does its first update happen on the next ComponentCollection update?

	- An I_Component which is to be removed during a ComponentCollection update was already updated
	  before the remove request was made during that update. Is this acceptable behavior?

Let's suppose that we do answer all of these questions and devise and implement an elegant and extensible
solution (which, by the way, is a non-trivial task). Even if we do this, we still have to remember all
of these rules and do lots of additional error checking, which will add a significant amount of complexity
to both the API and its implementation.

So, to avoid these issues, we disallow component addition and removal entirely during a
ComponentCollection update. (Personal Note: I should let this be an example as to why additional flexibility
is not always a good thing.)
*/

export namespace Brawler
{
	class ComponentCollection
	{
	public:
		explicit ComponentCollection(SceneNode& owningNode);

		ComponentCollection(const ComponentCollection& rhs) = delete;
		ComponentCollection& operator=(const ComponentCollection& rhs) = delete;

		ComponentCollection(ComponentCollection&& rhs) noexcept = default;
		ComponentCollection& operator=(ComponentCollection&& rhs) noexcept = default;

		/// <summary>
		/// Updates all of the I_Component instances within this ComponentCollection.
		/// </summary>
		/// <param name="dt">
		/// - The timestep (in milliseconds) for this update.
		/// </param>
		void Update(const float dt);

		template <typename T, typename... Args>
			requires std::derived_from<T, Brawler::I_Component>
		void CreateComponent(Args&&... args);

		/// <summary>
		/// Marks the I_Component of type T pointed to by componentPtr for removal from this
		/// ComponentCollection. The function asserts if componentPtr is not of type T or is 
		/// not owned by this ComponentCollection.
		/// 
		/// Internally, the component itself remains in memory until the next update. This
		/// means that it is safe for a component to remove itself, among other things.
		/// </summary>
		/// <typeparam name="T">
		/// - The derived I_Component type of the component being removed.
		/// </typeparam>
		/// <param name="componentPtr">
		/// - A pointer to the component which is to be removed.
		/// </param>
		/// <returns></returns>
		template <typename T, typename DecayedT = std::decay_t<T>>
			requires IMPL::IsValidComponent<T>
		void RemoveComponent(DecayedT* componentPtr);

		/// <summary>
		/// Returns the number of I_Component instances of type T which are owned by this
		/// ComponentCollection.
		/// </summary>
		/// <typeparam name="T">
		/// - The derived I_Component type for which the count will be returned.
		/// </typeparam>
		/// <returns>
		/// The number of I_Component instances of type T which are owned by this 
		/// ComponentCollection.
		/// </returns>
		template <typename T>
			requires std::derived_from<T, Brawler::I_Component>
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

	private:
		/// <summary>
		/// Runs the callback specified by cmd on every Brawler::I_Component instance in the
		/// collection.
		/// </summary>
		/// <param name="cmd">
		/// - The callback to run on the components.
		/// </param>
		template <Brawler::Function<void, Brawler::I_Component&> Func>
		void ExecuteComponentCommand(const Func& cmd);

		/// <summary>
		/// Runs the callback specified by cmd on every Brawler::I_Component instance in the
		/// collection.
		/// </summary>
		/// <param name="cmd">
		/// - The callback to run on the components.
		/// </param>
		template <Brawler::Function<void, const Brawler::I_Component&> Func>
		void ExecuteComponentCommand(const Func& cmd) const;

		void ExecutePendingRemovals();

	private:
		std::unordered_map<Brawler::ComponentID, std::vector<std::unique_ptr<Brawler::I_Component>>> mComponentMap;
		std::vector<std::unique_ptr<I_Component>> mComponentsPendingRemoval;

#ifdef _DEBUG
		bool mIsUpdating;
#endif // _DEBUG

		SceneNode* const mOwningNode;
	};
}

// --------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, typename... Args>
		requires std::derived_from<T, Brawler::I_Component>
	void ComponentCollection::CreateComponent(Args&&... args)
	{
		assert(!mIsUpdating && "ERROR: An attempt was made to add a component to a ComponentCollection while it was being updated!");

		const ComponentID compId{ Brawler::GetComponentID<T>() };

		std::unique_ptr<I_Component> compPtr{ std::make_unique<T>(std::forward<Args>(args)...) };
		compPtr->SetSceneNode(*mOwningNode);

		mComponentMap[compId].push_back(std::move(compPtr));
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	void ComponentCollection::RemoveComponent(DecayedT* componentPtr)
	{
		assert(!mIsUpdating && "ERROR: An attempt was made to remove a component from a ComponentCollection while it was being updated!");
		
		constexpr ComponentID compID{ Brawler::GetComponentID<DecayedT>() };

		if (mComponentMap.contains(compID))
		{
			for (auto itr = mComponentMap.at(compID).begin(); itr != mComponentMap.at(compID).end(); ++itr)
			{
				// If we find the pointer which we want to remove, then std::move() it into the array of
				// components pending removal and erase the std::unique_ptr instance which used to contain
				// it.
				if (componentPtr == static_cast<DecayedT>(itr->get()))
				{
					mComponentsPendingRemoval.insert(std::move(*itr));
					mComponentMap.at(compID).erase(itr);

					return;
				}
			}
		}

		// If we could not find the component, then assert.
		assert(false && "ERROR: ComponentCollection::RemoveComponent<T>() failed! (Either the type was specified incorrectly, or the ComponentCollection did not actually own this component.)");
	}

	template <typename T>
		requires std::derived_from<T, Brawler::I_Component>
	std::size_t ComponentCollection::GetComponentCount() const
	{
		constexpr std::vector<ComponentID> RELEVANT_IDS{ Brawler::GetCompatibleComponentIDs<T>() };
		std::size_t count = 0;

		for (const auto componentID : RELEVANT_IDS)
		{
			if (mComponentMap.contains(componentID))
				count += mComponentMap.at(componentID).size();
		}

		return count;
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	DecayedT* ComponentCollection::GetComponent()
	{
		// Do not allow GetComponent() to be called if more than one component of the same type is
		// present.
		assert(GetComponentCount<DecayedT>() <= 1 && 
			"ERROR: An attempt was made to call ComponentCollection::GetComponent<T>(), but the owning SceneNode had more than 1 component of type T! (In this case, you should use ComponentCollection::GetComponents<T>()!)");

		constexpr std::vector<ComponentID> RELEVANT_IDS{ Brawler::GetCompatibleComponentIDs<T>() };

		for (const auto compID : RELEVANT_IDS)
		{
			if (mComponentMap.contains(compID) && !mComponentMap.at(compID).empty())
				return static_cast<DecayedT*>((mComponentMap.at(compID))[0].get());
		}

		return nullptr;
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	const DecayedT* ComponentCollection::GetComponent() const
	{
		// Do not allow GetComponent() to be called if more than one component of the same type is
		// present.
		assert(GetComponentCount<DecayedT>() <= 1 &&
			"ERROR: An attempt was made to call ComponentCollection::GetComponent<T>(), but the owning SceneNode had more than 1 component of type T! (In this case, you should use ComponentCollection::GetComponents<T>()!)");

		constexpr std::vector<ComponentID> RELEVANT_IDS{ Brawler::GetCompatibleComponentIDs<T>() };

		for (const auto compID : RELEVANT_IDS)
		{
			if (mComponentMap.contains(compID) && !mComponentMap.at(compID).empty())
				return static_cast<DecayedT*>((mComponentMap.at(compID))[0].get());
		}

		return nullptr;
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	std::vector<DecayedT*> ComponentCollection::GetComponents()
	{
		const std::size_t componentCount = GetComponentCount<DecayedT>();
		if (componentCount == 0)
			return std::vector<DecayedT*>{};
		
		constexpr std::vector<ComponentID> RELEVANT_IDS{ Brawler::GetCompatibleComponentIDs<T>() };
		std::vector<DecayedT*> compPtrArr{};
		compPtrArr.reserve(componentCount);

		for (const auto compID : RELEVANT_IDS)
		{
			if (!mComponentMap.contains(compID))
				continue;

			for (auto& compPtr : mComponentMap.at(compID))
				compPtrArr.push_back(static_cast<DecayedT*>(compPtr.get()));
		}

		return compPtrArr;
	}

	template <typename T, typename DecayedT>
		requires IMPL::IsValidComponent<T>
	std::vector<const DecayedT*> ComponentCollection::GetComponents() const
	{
		const std::size_t componentCount = GetComponentCount<DecayedT>();
		if (componentCount == 0)
			return std::vector<DecayedT*>{};

		constexpr std::vector<ComponentID> RELEVANT_IDS{ Brawler::GetCompatibleComponentIDs<T>() };
		std::vector<DecayedT*> compPtrArr{};
		compPtrArr.reserve(componentCount);

		for (const auto compID : RELEVANT_IDS)
		{
			if (!mComponentMap.contains(compID))
				continue;

			for (const auto& compPtr : mComponentMap.at(compID))
				compPtrArr.push_back(static_cast<DecayedT*>(compPtr.get()));
		}

		return compPtrArr;
	}

	template <Brawler::Function<void, Brawler::I_Component&> Func>
	void ComponentCollection::ExecuteComponentCommand(const Func& cmd)
	{
		for (auto& itr : mComponentMap)
		{
			for (auto& componentPtr : itr.second)
				cmd(*(componentPtr.get()));
		}
	}

	template <Brawler::Function<void, const Brawler::I_Component&> Func>
	void ComponentCollection::ExecuteComponentCommand(const Func& cmd) const
	{
		for (const auto& itr : mComponentMap)
		{
			for (const auto& componentPtr : itr.second)
				cmd(*(componentPtr.get()));
		}
	}
}