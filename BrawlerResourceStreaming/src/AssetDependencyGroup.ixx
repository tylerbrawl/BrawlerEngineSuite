module;
#include <memory>
#include <vector>
#include <concepts>

export module Brawler.AssetDependencyGroup;
import Brawler.AssetHandle;
import Brawler.PersistentAssetDependencyGroup;

export namespace Brawler
{
	class I_Asset;
}

export namespace Brawler
{
	class AssetDependencyGroup
	{
	public:
		AssetDependencyGroup();

		AssetDependencyGroup(const AssetDependencyGroup& rhs) = delete;
		AssetDependencyGroup& operator=(const AssetDependencyGroup& rhs) = delete;

		AssetDependencyGroup(AssetDependencyGroup&& rhs) noexcept = default;
		AssetDependencyGroup& operator=(AssetDependencyGroup&& rhs) noexcept = default;

		template <typename T>
			requires std::derived_from<T, I_Asset>
		void AddAssetDependency(AssetHandle<T> hAsset);

		void AddAssetDependencyGroup(AssetDependencyGroup&& dependencyGroup);
		void AddAssetDependencyGroup(PersistentAssetDependencyGroup&& dependencyGroup);

		/// <summary>
		/// Before every update, every child within an AssetDependencyGroup is checked to see if
		/// it needs to have some data loaded or unloaded.
		/// 
		/// However, in levels with a large amount of assets, this can become a drain in performance,
		/// since the entire asset dependency graph would need to be traversed. Thus, if all of the
		/// assets within an AssetDependencyGroup (along with all of its child groups) only need to
		/// be loaded and unloaded once, rather than streamed, then PersistentAssetDependencyGroup
		/// is a better fit.
		/// 
		/// PersistentAssetDependencyGroup::CheckDependenciesForAssetDataRequests() will only check for
		/// asset load requests once. With proper usage, this can skip large sections of the asset
		/// dependency graph. However, one must be careful to *NOT* add streamed assets as a
		/// dependency to a PersistentAssetDependencyGroup.
		/// </summary>
		void CheckDependenciesForAssetDataRequests();

		bool AreDependenciesLoaded() const;

	protected:
		std::vector<std::unique_ptr<AssetDependencyGroup>> mChildGroups;
		std::vector<std::unique_ptr<PersistentAssetDependencyGroup>> mPersistentChildGroups;
		std::vector<I_Asset*> mAssetDependencies;
	};
}

// -----------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_Asset>
	void AssetDependencyGroup::AddAssetDependency(AssetHandle<T> hAsset)
	{
		I_Asset* assetPtr = static_cast<I_Asset*>(&(*hAsset));
		mAssetDependencies.push_back(assetPtr);
	}
}