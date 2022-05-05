module;
#include <memory>
#include <vector>

module Brawler.AssetDependencyGroup;
import Brawler.PersistentAssetDependencyGroup;
import Brawler.I_Asset;
import Brawler.JobSystem;

namespace Brawler
{
	AssetDependencyGroup::AssetDependencyGroup() :
		mChildGroups(),
		mPersistentChildGroups(),
		mAssetDependencies()
	{}

	void AssetDependencyGroup::AddAssetDependencyGroup(AssetDependencyGroup&& dependencyGroup)
	{
		std::unique_ptr<AssetDependencyGroup> groupPtr{ std::make_unique<AssetDependencyGroup>(std::move(dependencyGroup)) };
		mChildGroups.push_back(std::move(groupPtr));
	}

	void AssetDependencyGroup::AddAssetDependencyGroup(PersistentAssetDependencyGroup&& dependencyGroup)
	{
		std::unique_ptr<PersistentAssetDependencyGroup> groupPtr{ std::make_unique<PersistentAssetDependencyGroup>(std::move(dependencyGroup)) };
		mPersistentChildGroups.push_back(std::move(groupPtr));
	}

	void AssetDependencyGroup::CheckDependenciesForAssetDataRequests()
	{
		Brawler::JobGroup childUpdateChecks{};
		childUpdateChecks.Reserve(mAssetDependencies.size() + mPersistentChildGroups.size() + mChildGroups.size());

		for (auto& asset : mAssetDependencies)
		{
			childUpdateChecks.AddJob([asset] ()
			{
				asset->UpdateAssetDataIMPL();
			});
		}
		
		for (auto& persistentChildGroup : mPersistentChildGroups)
		{
			PersistentAssetDependencyGroup* const persistentChildGroupPtr = persistentChildGroup.get();
			childUpdateChecks.AddJob([persistentChildGroupPtr] ()
			{
				persistentChildGroupPtr->CheckDependenciesForAssetDataRequests();
			});
		}
		
		for (auto& childGroup : mChildGroups)
		{
			AssetDependencyGroup* const childGroupPtr = childGroup.get();
			childUpdateChecks.AddJob([childGroupPtr] ()
			{
				childGroupPtr->CheckDependenciesForAssetDataRequests();
			});
		}
		
		childUpdateChecks.ExecuteJobs();
	}

	bool AssetDependencyGroup::AreDependenciesLoaded() const
	{
		for (const auto& asset : mAssetDependencies)
		{
			if (!asset->IsLoaded())
				return false;
		}

		for (const auto& persistentChildGroup : mPersistentChildGroups)
		{
			if (!persistentChildGroup->AreDependenciesLoaded())
				return false;
		}

		for (const auto& childGroup : mChildGroups)
		{
			if (!childGroup->AreDependenciesLoaded())
				return false;
		}

		return true;
	}
}