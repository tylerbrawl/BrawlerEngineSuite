module;
#include <memory>
#include <vector>

export module Brawler.PersistentAssetDependencyGroup;
import Brawler.AssetHandle;

export namespace Brawler
{
	class I_PersistentAsset;
}

/*
You may notice that PersistentAssetDependencyGroup implements a lot of the same functionality that
AssetDependencyGroup does, but it does not inherit from it using polymorphism. There are two reasons
for this:

	1. We want it to be impossible to add AssetDependencyGroup instances which could potentially
	   contain streamable assets to a PersistentAssetDependencyGroup. This will significantly reduce 
	   the chances of API misuse and thus strange asset loading errors.

	2. We want it to be impossible to add streamable assets in general to a PersistentAssetDependencyGroup. 
	   This will also help protect against API misuse.
*/

export namespace Brawler
{
	class PersistentAssetDependencyGroup
	{
	public:
		PersistentAssetDependencyGroup();

		PersistentAssetDependencyGroup(const PersistentAssetDependencyGroup& rhs) = delete;
		PersistentAssetDependencyGroup& operator=(const PersistentAssetDependencyGroup& rhs) = delete;

		PersistentAssetDependencyGroup(PersistentAssetDependencyGroup&& rhs) noexcept = default;
		PersistentAssetDependencyGroup& operator=(PersistentAssetDependencyGroup&& rhs) noexcept = default;

		template <typename T>
			requires std::derived_from<T, I_PersistentAsset>
		void AddAssetDependency(AssetHandle<T> hAsset);

		void AddAssetDependencyGroup(PersistentAssetDependencyGroup&& dependencyGroup);

		/// <summary>
		/// Unlike AssetDependencyGroup, PersistentAssetDependencyGroup will only traverse child nodes to
		/// check for asset load requests once, regardless of how many times
		/// PersistentAssetDependencyGroup::CheckDependenciesForAssetDataRequests() is called.
		/// </summary>
		void CheckDependenciesForAssetDataRequests();

		bool AreDependenciesLoaded() const;

	private:
		std::vector<std::unique_ptr<PersistentAssetDependencyGroup>> mChildGroups;
		std::vector<I_PersistentAsset*> mAssetDependencies;
		bool mDependenciesChecked;
	};
}

// ---------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_PersistentAsset>
	void PersistentAssetDependencyGroup::AddAssetDependency(AssetHandle<T> hAsset)
	{
		I_PersistentAsset* assetPtr = static_cast<I_PersistentAsset*>(&(*hAsset));
		mAssetDependencies.push_back(assetPtr);
	}
}