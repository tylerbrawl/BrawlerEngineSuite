module;
#include <cstddef>
#include <cassert>
#include <DxDef.h>

module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Util.D3D12;
import Util.Engine;

namespace Brawler
{
	GlobalTexturePageUploadRequest::GlobalTexturePageUploadRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo) :
		VirtualTextureStreamingNotifier(logicalPage),
		mPageInfo(std::move(destPageInfo)),
		mPageDataCopySubAllocation()
	{
		assert(logicalPage.VirtualTexturePtr != nullptr);
		const D3D12_SUBRESOURCE_FOOTPRINT& pageDataFootprint{ logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetPageDataFootprint() };

		mPageDataCopySubAllocation = D3D12::TextureCopyBufferSubAllocation{ pageDataFootprint };
	}

	GlobalTexturePageUploadRequest::~GlobalTexturePageUploadRequest()
	{
		// If this request is for a combined page, then mark the first useable frame number for this 
		// VirtualTexture as the next frame number, calculated as (Util::Engine::GetCurrentFrameNumber() + 1). 
		// This essentially states that the data will be ready on the GPU during the next frame.
		if (GetLogicalPage().VirtualTexturePtr != nullptr) [[likely]]
		{
			const bool isCombinedPage = (GetLogicalPage().LogicalMipLevel >= GetLogicalPage().VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

			if (isCombinedPage)
				SetVirtualTextureFirstUseableFrameNumber(Util::Engine::GetCurrentFrameNumber() + 1);
		}
	}

	const GlobalTexturePageInfo& GlobalTexturePageUploadRequest::GetDestinationGlobalTexturePageInfo() const
	{
		return mPageInfo;
	}

	D3D12::TextureCopyBufferSubAllocation& GlobalTexturePageUploadRequest::GetPageDataBufferSubAllocation()
	{
		return mPageDataCopySubAllocation;
	}

	const D3D12::TextureCopyBufferSubAllocation& GlobalTexturePageUploadRequest::GetPageDataBufferSubAllocation() const
	{
		return mPageDataCopySubAllocation;
	}
}