module;
#include <unordered_map>
#include <optional>
#include <vector>
#include <cassert>

module Brawler.VirtualTextureActivePageTracker;

namespace Brawler
{
	void VirtualTextureActivePageTracker::UpdateTrackedPage(const TrackedPage trackedPage, const GlobalTexturePageIdentifier storagePageIdentifier)
	{
		mPageStorageMap[trackedPage] = storagePageIdentifier;
	}

	void VirtualTextureActivePageTracker::RemoveTrackedPage(const TrackedPage trackedPage)
	{
		assert(mPageStorageMap.contains(trackedPage));
		mPageStorageMap.erase(trackedPage);
	}

	std::vector<GlobalTexturePageIdentifier> VirtualTextureActivePageTracker::GetOccupiedGlobalTexturePages() const
	{
		std::vector<GlobalTexturePageIdentifier> occupiedIdentifierArr{};

		for (const auto& [trackedPage, pageIdentifier] : mPageStorageMap)
			occupiedIdentifierArr.push_back(pageIdentifier);

		return occupiedIdentifierArr;
	}

	std::optional<GlobalTexturePageIdentifier> VirtualTextureActivePageTracker::GetStoragePageIdentifier(const TrackedPage trackedPage) const
	{
		if (!mPageStorageMap.contains(trackedPage)) [[unlikely]]
			return std::optional<GlobalTexturePageIdentifier>{};

		return std::optional<GlobalTexturePageIdentifier>{ mPageStorageMap.at(trackedPage) };
	}
}