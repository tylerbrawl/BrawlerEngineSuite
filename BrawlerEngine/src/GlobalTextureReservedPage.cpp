module;
#include <cassert>

module Brawler.GlobalTextureReservedPage;

namespace Brawler
{
	bool GlobalTextureReservedPage::HasAllocation() const
	{
		return (mVirtualTexturePtr != nullptr);
	}

	void GlobalTextureReservedPage::ClearVirtualTexture()
	{
		mVirtualTexturePtr = nullptr;
		mLogicalPage = VirtualTextureLogicalPage{};
	}

	VirtualTexture& GlobalTextureReservedPage::GetVirtualTexture()
	{
		assert(HasAllocation());
		return *mVirtualTexturePtr;
	}

	const VirtualTexture& GlobalTextureReservedPage::GetVirtualTexture() const
	{
		assert(HasAllocation());
		return *mVirtualTexturePtr;
	}

	const VirtualTextureLogicalPage& GlobalTextureReservedPage::GetAllocatedLogicalPage() const
	{
		assert(HasAllocation());
		return mLogicalPage;
	}
}