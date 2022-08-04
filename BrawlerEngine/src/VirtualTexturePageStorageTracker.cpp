module;
#include <vector>
#include <cassert>
#include <ranges>

module Brawler.ActiveGlobalTexturePageDatabase;
import Util.General;

namespace Brawler
{
	void VirtualTexturePageStorageTracker::UpdateForGlobalTexturePageStorage(GlobalTexturePageSwapOperation& pageSwapOperation)
	{
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			const VirtualTextureLogicalPage& replacementLogicalPage{ pageSwapOperation.GetReplacementPage().GetAllocatedLogicalPage() };
			const GlobalTextureReservedPage& storagePage{ pageSwapOperation.GetStoragePage() };
			const VirtualTextureLogicalPage& storageLogicalPage{ storagePage.GetAllocatedLogicalPage() };

			assert(replacementLogicalPage.VirtualTexturePtr == storageLogicalPage.VirtualTexturePtr);
			assert(replacementLogicalPage.LogicalMipLevel == storageLogicalPage.LogicalMipLevel);
			assert(replacementLogicalPage.LogicalPageXCoordinate == storageLogicalPage.LogicalPageXCoordinate);
			assert(replacementLogicalPage.LogicalPageYCoordinate == storageLogicalPage.LogicalPageYCoordinate);

			for (const auto storagePagePtr : mStoragePagePtrArr)
				assert(storagePagePtr != &storagePage);
		}

		mStoragePagePtrArr.push_back(&(pageSwapOperation.GetStoragePage()));
	}

	void VirtualTexturePageStorageTracker::UpdateForGlobalTexturePageRemoval(GlobalTexturePageSwapOperation& pageSwapOperation)
	{
		const GlobalTextureReservedPage& storagePage{ pageSwapOperation.GetStoragePage() };
		const auto itr = std::ranges::find_if(mStoragePagePtrArr, [storagePagePtr = &storagePage] (const GlobalTextureReservedPage* const existingStoragePagePtr) { return (existingStoragePagePtr == storagePagePtr); });

		assert(itr != mStoragePagePtrArr.end());
		mStoragePagePtrArr.erase(itr);
	}

	void VirtualTexturePageStorageTracker::UpdateForVirtualTextureDeletion()
	{
		for (const auto storagePagePtr : mStoragePagePtrArr)
			storagePagePtr->ClearVirtualTexturePage();

		mStoragePagePtrArr.clear();
	}

	OptionalRef<GlobalTextureReservedPage> VirtualTexturePageStorageTracker::GetGlobalTextureStoragePage(const VirtualTextureLogicalPage& logicalPage)
	{
		for (const auto storagePagePtr : mStoragePagePtrArr)
		{
			const VirtualTextureLogicalPage& storageLogicalPage{ storagePagePtr->GetAllocatedLogicalPage() };
			assert(storageLogicalPage.VirtualTexturePtr == logicalPage.VirtualTexturePtr && "ERROR: A VirtualTexturePageStorageTracker was given a VirtualTextureLogicalPage instance referring to a different VirtualTexture in a call to VirtualTexturePageStorageTracker::GetGlobalTextureStoragePage()!");

			if (storageLogicalPage.LogicalMipLevel == logicalPage.LogicalMipLevel && storageLogicalPage.LogicalPageXCoordinate == logicalPage.LogicalPageXCoordinate &&
				storageLogicalPage.LogicalPageYCoordinate == logicalPage.LogicalPageYCoordinate)
				return Brawler::OptionalRef<GlobalTextureReservedPage>{ *storagePagePtr };
		}

		return Brawler::OptionalRef<GlobalTextureReservedPage>{};
	}

	OptionalRef<const GlobalTextureReservedPage> VirtualTexturePageStorageTracker::GetGlobalTextureStoragePage(const VirtualTextureLogicalPage& logicalPage) const
	{
		for (const auto storagePagePtr : mStoragePagePtrArr)
		{
			const VirtualTextureLogicalPage& storageLogicalPage{ storagePagePtr->GetAllocatedLogicalPage() };
			assert(storageLogicalPage.VirtualTexturePtr == logicalPage.VirtualTexturePtr && "ERROR: A VirtualTexturePageStorageTracker was given a VirtualTextureLogicalPage instance referring to a different VirtualTexture in a call to VirtualTexturePageStorageTracker::GetGlobalTextureStoragePage()!");

			if (storageLogicalPage.LogicalMipLevel == logicalPage.LogicalMipLevel && storageLogicalPage.LogicalPageXCoordinate == logicalPage.LogicalPageXCoordinate &&
				storageLogicalPage.LogicalPageYCoordinate == logicalPage.LogicalPageYCoordinate)
				return Brawler::OptionalRef<const GlobalTextureReservedPage>{ *storagePagePtr };
		}

		return Brawler::OptionalRef<const GlobalTextureReservedPage>{};
	}
}