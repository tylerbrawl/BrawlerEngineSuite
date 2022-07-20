module;
#include <cassert>
#include <cstdint>

module Brawler.GlobalTextureReservedPage;
import Util.Engine;

namespace Brawler
{
	bool GlobalTextureReservedPage::HasAllocation() const
	{
		return (mLogicalPage.VirtualTexturePtr != nullptr);
	}

	void GlobalTextureReservedPage::ClearVirtualTexturePage()
	{
		mLogicalPage = VirtualTextureLogicalPage{};
		mPageMetadataPtr = nullptr;
	}

	VirtualTexture& GlobalTextureReservedPage::GetVirtualTexture()
	{
		assert(HasAllocation());
		return *(mLogicalPage.VirtualTexturePtr);
	}

	const VirtualTexture& GlobalTextureReservedPage::GetVirtualTexture() const
	{
		assert(HasAllocation());
		return *(mLogicalPage.VirtualTexturePtr);
	}

	const VirtualTextureLogicalPage& GlobalTextureReservedPage::GetAllocatedLogicalPage() const
	{
		assert(HasAllocation());
		return mLogicalPage;
	}

	const VirtualTexturePageMetadata& GlobalTextureReservedPage::GetAllocatedPageMetadata() const
	{
		assert(HasAllocation());
		return *mPageMetadataPtr;
	}

	VirtualTexturePageType GlobalTextureReservedPage::GetAllocatedPageType() const
	{
		if (!HasAllocation()) [[unlikely]]  // We use [[unlikely]] because the function probably isn't going to be called unless an allocation exists.
			return VirtualTexturePageType::NO_PAGE_ALLOCATED;

		return (mLogicalPage.LogicalMipLevel >= mLogicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage() ? VirtualTexturePageType::COMBINED_PAGE : VirtualTexturePageType::OTHER);
	}

	void GlobalTextureReservedPage::NotifyUsageInCurrentFrame()
	{
		mLastUsedFrameNumber = Util::Engine::GetCurrentFrameNumber();
	}

	std::uint64_t GlobalTextureReservedPage::GetLastUsedFrameNumber() const
	{
		return mLastUsedFrameNumber;
	}
}