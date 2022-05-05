module;
#include <variant>

export module Brawler.AssetDependencyTracker;
import Brawler.AssetDependencyGroup;
import Brawler.PersistentAssetDependencyGroup;

export namespace Brawler
{
	class AssetManager;
}

export namespace Brawler
{
	class AssetDependencyTracker
	{
	private:
		friend class AssetManager;

	private:
		explicit AssetDependencyTracker(const AssetDependencyGroup& dependencyGroup);
		explicit AssetDependencyTracker(const PersistentAssetDependencyGroup& persistentGroup);

	public:
		/// <summary>
		/// Checks if all of the assets and child asset dependency groups belonging to the
		/// asset dependency group corresponding to this AssetDependencyTracker instance
		/// have been fully loaded.
		/// 
		/// An I_Asset is considered fully loaded if all of its asset data has been loaded
		/// and it will not submit any additional asset data load requests until at least
		/// some of its asset data has been unloaded. In terms of the API, an I_Asset is
		/// fully loaded if I_Asset::IsLoaded() returns true.
		/// 
		/// An asset dependency group (persistent or not) is considered fully loaded if
		/// all of its child I_Asset instances and asset dependency groups are fully
		/// loaded.
		/// </summary>
		/// <returns>
		/// This function returns true if all of the child I_Asset instances starting from
		/// this asset dependency group in the asset dependency graph have been loaded and
		/// false otherwise.
		/// </returns>
		bool AreAssetsLoaded() const;

	private:
		std::variant<
			const AssetDependencyGroup*,
			const PersistentAssetDependencyGroup*
		> mDependencyGroupPtr;
	};
}