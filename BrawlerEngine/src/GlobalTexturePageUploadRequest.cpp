module;
#include <cstddef>
#include <cassert>
#include <DxDef.h>

module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Util.D3D12;

namespace Brawler
{
	GlobalTexturePageUploadRequest::GlobalTexturePageUploadRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo) :
		mLogicalPage(logicalPage),
		mPageInfo(std::move(destPageInfo)),
		mPageDataCopySubAllocation()
	{
		assert(logicalPage.VirtualTexturePtr != nullptr);
		const D3D12_SUBRESOURCE_FOOTPRINT& pageDataFootprint{ logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetPageDataFootprint() };

		mPageDataCopySubAllocation = D3D12::TextureCopyBufferSubAllocation{ pageDataFootprint };
	}

	const VirtualTextureLogicalPage& GlobalTexturePageUploadRequest::GetLogicalPage() const
	{
		return mLogicalPage;
	}

	const GlobalTexturePageInfo& GlobalTexturePageUploadRequest::GetDestinationGlobalTexturePageInfo() const
	{
		return mPageInfo;
	}

	D3D12::TextureCopyBufferSubAllocation& GlobalTexturePageUploadRequest::GetPageDataBufferSubAllocation() const
	{
		return mPageDataCopySubAllocation;
	}
}