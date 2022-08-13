module;
#include <cstddef>
#include <cassert>

module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;

namespace Brawler
{
	GlobalTexturePageUploadRequest::GlobalTexturePageUploadRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo) :
		mLogicalPage(logicalPage),
		mPageInfo(std::move(destPageInfo)),
		mTextureDataSubAllocation()
	{
		assert(logicalPage.VirtualTexturePtr != nullptr);
		const std::size_t pageDataCopyableFootprintSize = logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetCopyableFootprintsPageSize();

		mTextureDataSubAllocation = PageDataSubAllocation{ pageDataCopyableFootprintSize };
	}

	const VirtualTextureLogicalPage& GlobalTexturePageUploadRequest::GetLogicalPage() const
	{
		return mLogicalPage;
	}

	const GlobalTexturePageInfo& GlobalTexturePageUploadRequest::GetDestinationGlobalTexturePageInfo() const
	{
		return mPageInfo;
	}

	GlobalTexturePageUploadRequest::PageDataSubAllocation& GlobalTexturePageUploadRequest::GetPageDataBufferSubAllocation() const
	{
		return mTextureDataSubAllocation;
	}
}