module;
#include <mutex>
#include <vector>
#include <span>
#include <memory>

module Brawler.VirtualTextureManagementSubModule;

namespace Brawler
{
	void GlobalTexturePageTransferSubModule::AddPageTransferRequests(const std::span<std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan)
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };

		for (auto&& transferRequestPtr : transferRequestSpan)
			mTransferRequestPtrArr.push_back(std::move(transferRequestPtr));
	}

	bool GlobalTexturePageTransferSubModule::HasPageTransferRequests() const
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };
		return !mTransferRequestPtrArr.empty();
	}

	std::vector<GlobalTexturePageTransferSubModule::PageTransferRenderPass_T> GlobalTexturePageTransferSubModule::GetPageTransferRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> currRequestArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };
			currRequestArr = std::move(mTransferRequestPtrArr);
		}

		if (currRequestArr.empty())
			return std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>>{};

		// In order to copy page data between GlobalTextures, we could potentially make use of
		// ID3D12GraphicsCommandList::CopyTextureRegion() to copy between GlobalTextures, rather
		// than use a working buffer. However, there are some issues with this approach:
		//
		//   - Suppose we are transferring from one location within a GlobalTexture to another
		//     location within the same GlobalTexture. We wouldn't be able to do the copy like
		//     that because we would not be able to put the GlobalTexture in both the
		//     D3D12_RESOURCE_STATE_COPY_SOURCE and the D3D12_RESOURCE_STATE_COPY_DEST states,
		//     so we would have to use a buffer. (Admittedly, this case would be quite pointless
		//     for decreasing memory consumption, since GlobalTextures do not suffer from internal
		//     fragmentation. However, moving texels within a GlobalTexture closer to each other
		//     could potentially improve cache hit rates during texture sampling.)
		//
		//   - 
	}
}