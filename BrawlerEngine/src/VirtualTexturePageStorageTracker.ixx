module;
#include <vector>

export module Brawler.ActiveGlobalTexturePageDatabase:VirtualTexturePageStorageTracker;
import Brawler.GlobalTextureReservedPage;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageSwapOperation;
import Brawler.OptionalRef;

export namespace Brawler
{
	class VirtualTexturePageStorageTracker
	{
	public:
		VirtualTexturePageStorageTracker() = default;

		VirtualTexturePageStorageTracker(const VirtualTexturePageStorageTracker& rhs) = delete;
		VirtualTexturePageStorageTracker& operator=(const VirtualTexturePageStorageTracker& rhs) = delete;

		VirtualTexturePageStorageTracker(VirtualTexturePageStorageTracker&& rhs) noexcept = default;
		VirtualTexturePageStorageTracker& operator=(VirtualTexturePageStorageTracker&& rhs) noexcept = default;

		void UpdateForGlobalTexturePageStorage(GlobalTexturePageSwapOperation& pageSwapOperation);
		void UpdateForGlobalTexturePageRemoval(GlobalTexturePageSwapOperation& pageSwapOperation);

		void UpdateForVirtualTextureDeletion();

		OptionalRef<GlobalTextureReservedPage> GetGlobalTextureStoragePage(const VirtualTextureLogicalPage& logicalPage);
		OptionalRef<const GlobalTextureReservedPage> GetGlobalTextureStoragePage(const VirtualTextureLogicalPage& logicalPage) const;

	private:
		/// <summary>
		/// For right now, we store all of the GlobalTextureReservedPage* values
		/// referring to pages in a GlobalTexture whose contents are those of a VirtualTexture
		/// page in a single linear array. We then have to do a search every time we want
		/// to find the page which stores the contents of a given VirtualTextureLogicalPage.
		/// 
		/// I don't expect there to be that many instances per VirtualTexture, so this
		/// is probably fine. If it turns out to be a performance bottleneck, then we should
		/// organize the data to search first by logical mip level and then by logical page
		/// coordinates.
		/// </summary>
		std::vector<GlobalTextureReservedPage*> mStoragePagePtrArr;
	};
}