module;

module Brawler.GlobalTexturePageRemovalRequest;

namespace Brawler
{
	GlobalTexturePageRemovalRequest::GlobalTexturePageRemovalRequest(const VirtualTextureLogicalPage& logicalPage) :
		VirtualTextureStreamingNotifier(logicalPage)
	{}
}