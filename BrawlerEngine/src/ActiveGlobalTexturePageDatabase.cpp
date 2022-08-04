module;
#include <unordered_map>
#include <span>
#include <memory>

module Brawler.ActiveGlobalTexturePageDatabase;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTextureDatabase;

namespace Brawler
{
	void ActiveGlobalTexturePageDatabase::UpdateForGlobalTexturePageSwaps(const std::span<const std::unique_ptr<GlobalTexturePageSwapOperation>> pageSwapPtrSpan)
	{
		for (const auto& pageSwapPtr : pageSwapPtrSpan)
		{
			if (pageSwapPtr->IsReplacingOlderPage())
			{
				const VirtualTextureLogicalPage& oldLogicalPage{ pageSwapPtr->GetPreviousPage().GetAllocatedLogicalPage() };

				if (mStorageTrackerMap.contains(oldLogicalPage.VirtualTexturePtr)) [[likely]]
					mStorageTrackerMap.at(oldLogicalPage.VirtualTexturePtr).UpdateForGlobalTexturePageRemoval(*pageSwapPtr);
			}

			const VirtualTextureLogicalPage& newLogicalPage{ pageSwapPtr->GetReplacementPage().GetAllocatedLogicalPage() };
			mStorageTrackerMap[newLogicalPage.VirtualTexturePtr].UpdateForGlobalTexturePageStorage(*pageSwapPtr);
		}
	}

	void ActiveGlobalTexturePageDatabase::UpdateForVirtualTextureDeletion(VirtualTexture& virtualTexture)
	{
		if (!mStorageTrackerMap.contains(&virtualTexture)) [[unlikely]]
			return;

		mStorageTrackerMap.at(&virtualTexture).UpdateForVirtualTextureDeletion();
		mStorageTrackerMap.erase(&virtualTexture);
	}

	OptionalRef<GlobalTextureReservedPage> ActiveGlobalTexturePageDatabase::GetGlobalTextureStoragePage(const VirtualTextureFeedback feedback)
	{
		VirtualTexture& virtualTexture{ VirtualTextureDatabase::GetInstance().GetVirtualTexture(feedback.VirtualTextureID) };
		
		if (!mStorageTrackerMap.contains(&virtualTexture)) [[unlikely]]
			return Brawler::OptionalRef<GlobalTextureReservedPage>{};

		const VirtualTextureLogicalPage referencedLogicalPage{
			.VirtualTexturePtr = &virtualTexture,
			.LogicalMipLevel = feedback.LogicalMipLevel,
			.LogicalPageXCoordinate = feedback.LogicalPageXCoordinate,
			.LogicalPageYCoordinate = feedback.LogicalPageYCoordinate
		};
		return mStorageTrackerMap.at(&virtualTexture).GetGlobalTextureStoragePage(referencedLogicalPage);
	}

	OptionalRef<const GlobalTextureReservedPage> ActiveGlobalTexturePageDatabase::GetGlobalTextureStoragePage(const VirtualTextureFeedback feedback) const
	{
		VirtualTexture& virtualTexture{ VirtualTextureDatabase::GetInstance().GetVirtualTexture(feedback.VirtualTextureID) };

		if (!mStorageTrackerMap.contains(&virtualTexture)) [[unlikely]]
			return Brawler::OptionalRef<const GlobalTextureReservedPage>{};

		const VirtualTextureLogicalPage referencedLogicalPage{
			.VirtualTexturePtr = &virtualTexture,
			.LogicalMipLevel = feedback.LogicalMipLevel,
			.LogicalPageXCoordinate = feedback.LogicalPageXCoordinate,
			.LogicalPageYCoordinate = feedback.LogicalPageYCoordinate
		};
		return mStorageTrackerMap.at(&virtualTexture).GetGlobalTextureStoragePage(referencedLogicalPage);
	}
}