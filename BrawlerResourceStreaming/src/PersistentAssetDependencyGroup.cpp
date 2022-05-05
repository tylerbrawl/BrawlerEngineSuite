module;
#include <memory>
#include <vector>

module Brawler.PersistentAssetDependencyGroup;
import Brawler.I_PersistentAsset;
import Brawler.JobSystem;

namespace Brawler
{
	PersistentAssetDependencyGroup::PersistentAssetDependencyGroup() :
		mChildGroups(),
		mAssetDependencies(),
		mDependenciesChecked(false)
	{}

	void PersistentAssetDependencyGroup::AddAssetDependencyGroup(PersistentAssetDependencyGroup&& dependencyGroup)
	{
		std::unique_ptr<PersistentAssetDependencyGroup> groupPtr{ std::make_unique<PersistentAssetDependencyGroup>(std::move(dependencyGroup)) };
		mChildGroups.push_back(std::move(groupPtr));
	}

	void PersistentAssetDependencyGroup::CheckDependenciesForAssetDataRequests()
	{
		// We need to check for asset load requests at least once.
		
		if (!mDependenciesChecked) [[unlikely]]
		{
			Brawler::JobGroup childUpdateChecks{};
			childUpdateChecks.Reserve(mAssetDependencies.size() + mChildGroups.size());

			for (auto& asset : mAssetDependencies)
			{
				childUpdateChecks.AddJob([asset] ()
				{
					asset->UpdateAssetDataIMPL();
				});
			}

			for (auto& childGroup : mChildGroups)
			{
				PersistentAssetDependencyGroup* const childGroupPtr = childGroup.get();
				childUpdateChecks.AddJob([childGroupPtr] ()
				{
					childGroupPtr->CheckDependenciesForAssetDataRequests();
				});
			}

			childUpdateChecks.ExecuteJobs();
			mDependenciesChecked = true;
		}
	}

	bool PersistentAssetDependencyGroup::AreDependenciesLoaded() const
	{
		for (const auto& asset : mAssetDependencies)
		{
			if (!asset->IsLoaded())
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