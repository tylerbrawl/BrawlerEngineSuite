module;
#include <memory>
#include <array>
#include <cassert>
#include <mutex>

module Brawler.VirtualTextureDatabase;
import Util.Engine;

namespace Brawler
{
	VirtualTextureDatabase& VirtualTextureDatabase::GetInstance()
	{
		static VirtualTextureDatabase instance{};
		return instance;
	}

	VirtualTextureHandle VirtualTextureDatabase::CreateVirtualTexture(const FilePathHash bvtxFileHash)
	{
		std::unique_ptr<VirtualTexture> virtualTexturePtr{ std::make_unique<VirtualTexture>(bvtxFileHash) };
		VirtualTextureHandle hVirtualTexture{ *virtualTexturePtr };

		// The constructor of VirtualTexture will attempt to reserve the VirtualTexture instance an
		// element in the VirtualTextureDescription GPUSceneBuffer; its index within this buffer becomes
		// its ID, and tells us where we should place the VirtualTexture instance within 
		// mVirtualTexturePtrArr.
		// 
		// This makes it easy to correlate virtual textures between the CPU and the GPU. If we find
		// a particular virtual texture ID within the buffer written to during the feedback pass, we
		// know exactly which VirtualTexture instance we are dealing with, as well as how to get access
		// to it.
		assert(virtualTexturePtr->GetVirtualTextureID() < mVirtualTexturePtrArr.size());
		VirtualTextureStorage& currStorage{ mVirtualTexturePtrArr[virtualTexturePtr->GetVirtualTextureID()] };

		{
			const std::scoped_lock<std::mutex> lock{ currStorage.CritSection };

			assert(currStorage.VirtualTexturePtr == nullptr);
			currStorage.VirtualTexturePtr = std::move(virtualTexturePtr);
		}

		return std::move(hVirtualTexture);
	}

	void VirtualTextureDatabase::DeleteVirtualTexture(VirtualTextureHandle& hVirtualTexture)
	{
		std::unique_ptr<VirtualTexture> extractedTexturePtr{};
		VirtualTextureStorage& currStorage{ mVirtualTexturePtrArr[hVirtualTexture->GetVirtualTextureID()] };

		{
			const std::scoped_lock<std::mutex> lock{ currStorage.CritSection };

			extractedTexturePtr = std::move(currStorage.VirtualTexturePtr);
		}
		
		mPendingDeletionArr.PushBack(std::move(extractedTexturePtr));
		hVirtualTexture.mVirtualTexturePtr = nullptr;
	}

	void VirtualTextureDatabase::TryCleanDeletedVirtualTextures()
	{
		mPendingDeletionArr.EraseIf([] (const std::unique_ptr<VirtualTexture>& virtualTexturePtr) { return virtualTexturePtr->SafeToDelete(); });
	}
}