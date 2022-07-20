module;
#include <memory>
#include <array>
#include <cassert>

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
		//
		// As an added bonus, since each VirtualTexture instance gets its own ID, and since an ID only
		// becomes available again after a VirtualTexture instance is destroyed, this function is
		// inherently thread safe.
		assert(virtualTexturePtr->GetVirtualTextureID() < mVirtualTexturePtrArr.size());
		mVirtualTexturePtrArr[virtualTexturePtr->GetVirtualTextureID()] = std::move(virtualTexturePtr);

		return std::move(hVirtualTexture);
	}

	void VirtualTextureDatabase::DeleteVirtualTexture(VirtualTextureHandle& hVirtualTexture)
	{
		mPendingDeletionArr.PushBack(PendingVirtualTextureDeletion{
			.VirtualTexturePtr{ std::move(mVirtualTexturePtrArr[hVirtualTexture->GetVirtualTextureID()]) },
			.DeletionFrameNumber = (Util::Engine::GetCurrentFrameNumber() + Util::Engine::MAX_FRAMES_IN_FLIGHT)
		});
		
		hVirtualTexture.mVirtualTexturePtr = nullptr;
	}
}