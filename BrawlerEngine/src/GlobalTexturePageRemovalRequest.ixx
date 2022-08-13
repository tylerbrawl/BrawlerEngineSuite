module;

export module Brawler.GlobalTexturePageRemovalRequest;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTextureStreamingNotifier;

export namespace Brawler
{
	class GlobalTexturePageRemovalRequest final : public VirtualTextureStreamingNotifier
	{
	public:
		GlobalTexturePageRemovalRequest() = default;
		explicit GlobalTexturePageRemovalRequest(const VirtualTextureLogicalPage& logicalPage);

		GlobalTexturePageRemovalRequest(const GlobalTexturePageRemovalRequest& rhs) = delete;
		GlobalTexturePageRemovalRequest& operator=(const GlobalTexturePageRemovalRequest& rhs) = delete;

		GlobalTexturePageRemovalRequest(GlobalTexturePageRemovalRequest&& rhs) noexcept = default;
		GlobalTexturePageRemovalRequest& operator=(GlobalTexturePageRemovalRequest&& rhs) noexcept = default;
	};
}