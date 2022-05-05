module;
#include <variant>

module Brawler.AssetDependencyTracker;

namespace Brawler
{
	AssetDependencyTracker::AssetDependencyTracker(const AssetDependencyGroup& dependencyGroup) :
		mDependencyGroupPtr(&dependencyGroup)
	{}

	AssetDependencyTracker::AssetDependencyTracker(const PersistentAssetDependencyGroup& persistentGroup) :
		mDependencyGroupPtr(&persistentGroup)
	{}

	bool AssetDependencyTracker::AreAssetsLoaded() const
	{
		if (mDependencyGroupPtr.index() == 0)
			return std::get<const AssetDependencyGroup*>(mDependencyGroupPtr)->AreDependenciesLoaded();

		return std::get<const PersistentAssetDependencyGroup*>(mDependencyGroupPtr)->AreDependenciesLoaded();
	}
}