module;
#include <utility>

module Brawler.GlobalTexturePageTransferRequest;

namespace Brawler
{
	GlobalTexturePageTransferRequest::GlobalTexturePageTransferRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo, GlobalTexturePageInfo&& srcPageInfo) :
		VirtualTextureStreamingNotifier(logicalPage),
		mDestPageInfo(std::move(destPageInfo)),
		mSrcPageInfo(std::move(srcPageInfo))
	{}

	const GlobalTexturePageInfo& GlobalTexturePageTransferRequest::GetDestinationGlobalTexturePageInfo() const
	{
		return mDestPageInfo;
	}

	const GlobalTexturePageInfo& GlobalTexturePageTransferRequest::GetSourceGlobalTexturePageInfo() const
	{
		return mSrcPageInfo;
	}
}