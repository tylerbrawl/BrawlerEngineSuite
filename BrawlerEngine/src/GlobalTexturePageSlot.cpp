module;
#include <optional>
#include <cassert>

module Brawler.GlobalTexturePageSlot;

namespace Brawler
{
	void GlobalTexturePageSlot::AddVirtualTexturePage(const VirtualTextureLogicalPage& logicalPage)
	{
		assert(!mStoredLogicalPage.has_value() && "ERROR: An attempt was made to store a VirtualTextureLogicalPage within a GlobalTexturePageSlot, but said slot was already storing page data!");
		mStoredLogicalPage = logicalPage;
	}

	VirtualTextureLogicalPage GlobalTexturePageSlot::RemoveVirtualTexturePage()
	{
		assert(mStoredLogicalPage.has_value() && "ERROR: An attempt was made to remove a VirtualTextureLogicalPage from a GlobalTexturePageSlot which wasn't storing one!");
		
		const VirtualTextureLogicalPage returnedPage{ std::move(*mStoredLogicalPage) };
		mStoredLogicalPage.reset();

		return returnedPage;
	}

	bool GlobalTexturePageSlot::HasVirtualTexturePage() const
	{
		return mStoredLogicalPage.has_value();
	}

	const VirtualTextureLogicalPage& GlobalTexturePageSlot::GetVirtualTexturePage() const
	{
		assert(HasVirtualTexturePage() && "ERROR: An attempt was made to get a const& to the VirtualTextureLogicalPage of a GlobalTexturePageSlot which wasn't storing one!");
		return *mStoredLogicalPage;
	}
}