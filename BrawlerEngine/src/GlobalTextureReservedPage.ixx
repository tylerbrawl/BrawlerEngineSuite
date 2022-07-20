module;
#include <type_traits>

export module Brawler.GlobalTextureReservedPage;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTextureMetadata;

export namespace Brawler
{
	enum class VirtualTexturePageType
	{
		/// <summary>
		/// A combined page contains all of the mip levels of a virtual texture whose
		/// dimensions are smaller than that of a page. These mip levels are packed into
		/// a single page, and an algorithm was developed to get the coordinates of each
		/// mip level's origin in constant time.
		/// 
		/// Every virtual texture must at least have its combined page present in a global
		/// texture. Otherwise, the system will assert in Debug builds. Additional global
		/// textures can be added to a GlobalTextureCollection for a given format.
		/// </summary>
		COMBINED_PAGE,

		OTHER,
		NO_PAGE_ALLOCATED
	};
}

export namespace Brawler
{
	class GlobalTextureReservedPage
	{
	public:
		GlobalTextureReservedPage() = default;

		GlobalTextureReservedPage(const GlobalTextureReservedPage& rhs) = default;
		GlobalTextureReservedPage& operator=(const GlobalTextureReservedPage& rhs) = default;

		GlobalTextureReservedPage(GlobalTextureReservedPage&& rhs) noexcept = default;
		GlobalTextureReservedPage& operator=(GlobalTextureReservedPage&& rhs) noexcept = default;

		bool HasAllocation() const;

		template <typename T>
			requires std::is_same_v<std::decay_t<T>, VirtualTextureLogicalPage>
		void SetVirtualTexturePage(T&& logicalPage);

		void ClearVirtualTexturePage();

		VirtualTexture& GetVirtualTexture();
		const VirtualTexture& GetVirtualTexture() const;

		const VirtualTextureLogicalPage& GetAllocatedLogicalPage() const;
		const VirtualTexturePageMetadata& GetAllocatedPageMetadata() const;

		VirtualTexturePageType GetAllocatedPageType() const;

		void NotifyUsageInCurrentFrame();
		std::uint64_t GetLastUsedFrameNumber() const;

	private:
		VirtualTextureLogicalPage mLogicalPage;

		/// <summary>
		/// VirtualTextureMetadata::GetPageMetadata() has to calculate the index of the relevant
		/// metadata within its internal array every time it is called. So, to avoid this, we
		/// store a pointer here.
		/// </summary>
		const VirtualTexturePageMetadata* mPageMetadataPtr;

		std::uint64_t mLastUsedFrameNumber;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::is_same_v<std::decay_t<T>, VirtualTextureLogicalPage>
	void GlobalTextureReservedPage::SetVirtualTexturePage(T&& logicalPage)
	{
		mLogicalPage = std::forward<T>(logicalPage);
		mPageMetadataPtr = &(mLogicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetPageMetadata(mLogicalPage.LogicalMipLevel, mLogicalPage.LogicalPageXCoordinate, mLogicalPage.LogicalPageYCoordinate));
	}
}