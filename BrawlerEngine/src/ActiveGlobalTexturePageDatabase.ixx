module;
#include <unordered_map>
#include <span>
#include <memory>

export module Brawler.ActiveGlobalTexturePageDatabase;
import :VirtualTexturePageStorageTracker;
import Brawler.VirtualTexture;
import Brawler.GlobalTexturePageSwapOperation;
import Brawler.OptionalRef;
import Brawler.GlobalTextureReservedPage;
import Brawler.VirtualTextureFeedback;

export namespace Brawler
{
	class ActiveGlobalTexturePageDatabase
	{
	public:
		ActiveGlobalTexturePageDatabase() = default;

		ActiveGlobalTexturePageDatabase(const ActiveGlobalTexturePageDatabase& rhs) = delete;
		ActiveGlobalTexturePageDatabase& operator=(const ActiveGlobalTexturePageDatabase& rhs) = delete;

		ActiveGlobalTexturePageDatabase(ActiveGlobalTexturePageDatabase&& rhs) noexcept = default;
		ActiveGlobalTexturePageDatabase& operator=(ActiveGlobalTexturePageDatabase&& rhs) noexcept = default;

		void UpdateForGlobalTexturePageSwaps(const std::span<const std::unique_ptr<GlobalTexturePageSwapOperation>> pageSwapPtrSpan);
		void UpdateForVirtualTextureDeletion(VirtualTexture& virtualTexture);

		OptionalRef<GlobalTextureReservedPage> GetGlobalTextureStoragePage(const VirtualTextureFeedback feedback);
		OptionalRef<const GlobalTextureReservedPage> GetGlobalTextureStoragePage(const VirtualTextureFeedback feedback) const;

	private:
		std::unordered_map<VirtualTexture*, VirtualTexturePageStorageTracker> mStorageTrackerMap;
	};
}