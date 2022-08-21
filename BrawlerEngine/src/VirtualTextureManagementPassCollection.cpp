module;
#include <vector>

module Brawler.VirtualTextureManagementPassCollection;

namespace Brawler
{
	void VirtualTextureManagementPassCollection::SetGlobalTextureUploadPasses(UploadPassCollection_T&& uploadPassCollection)
	{
		mUploadPassCollection = std::move(uploadPassCollection);
	}

	void VirtualTextureManagementPassCollection::SetGlobalTextureRemovalPasses(std::vector<RemovalPass_T>&& removalPassArr)
	{
		mRemovalPassArr = std::move(removalPassArr);
	}

	void VirtualTextureManagementPassCollection::SetGlobalTextureTransferPasses(TransferPassCollection_T&& transferPassCollection)
	{
		mTransferPassCollection = std::move(transferPassCollection);
	}

	void VirtualTextureManagementPassCollection::MoveRenderPassesIntoBundle(D3D12::RenderPassBundle& bundle)
	{
		// The order in which we add the passes determines the order in which they are executed on the
		// GPU... well, mostly, anyways. Since the passes are split amongst different queues, we can't
		// enforce a complete ordering, but we also don't need to.
		//
		// We really just want to make sure that transfer passes occur before upload passes do. Otherwise,
		// we might be accidentally duplicating data within a GlobalTexture. Removal passes can occur at
		// any time, since they are only generated for a VirtualTextureLogicalPage if said page is being
		// removed from all GlobalTexture instances.

		// GlobalTexture Removal Passes
		{
			for (auto&& removalPass : mRemovalPassArr)
				bundle.AddRenderPass(std::move(removalPass));
		}

		// GlobalTexture Transfer Passes
		{
			for (auto&& textureCopyPass : mTransferPassCollection.GlobalTextureCopyArr)
				bundle.AddRenderPass(std::move(textureCopyPass));

			for (auto&& indirectionTextureUpdatePass : mTransferPassCollection.IndirectionTextureUpdateArr)
				bundle.AddRenderPass(std::move(indirectionTextureUpdatePass));
		}
		
		// GlobalTexture Update Passes
		{
			for (auto&& textureCopyPass : mUploadPassCollection.GlobalTextureCopyPassArr)
				bundle.AddRenderPass(std::move(textureCopyPass));

			for (auto&& indirectionTextureUpdatePass : mUploadPassCollection.IndirectionTextureUpdatePassArr)
				bundle.AddRenderPass(std::move(indirectionTextureUpdatePass));
		}
	}
}