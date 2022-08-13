module;

export module Brawler.GlobalTexturePageTransferRequest;
import Brawler.VirtualTextureStreamingNotifier;
import Brawler.GlobalTexturePageInfo;
import Brawler.VirtualTextureLogicalPage;

export namespace Brawler
{
	class GlobalTexturePageTransferRequest final : public VirtualTextureStreamingNotifier
	{
	public:
		GlobalTexturePageTransferRequest() = default;
		GlobalTexturePageTransferRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo, GlobalTexturePageInfo&& srcPageInfo);

		GlobalTexturePageTransferRequest(const GlobalTexturePageTransferRequest& rhs) = delete;
		GlobalTexturePageTransferRequest& operator=(const GlobalTexturePageTransferRequest& rhs) = delete;

		GlobalTexturePageTransferRequest(GlobalTexturePageTransferRequest&& rhs) noexcept = default;
		GlobalTexturePageTransferRequest& operator=(GlobalTexturePageTransferRequest&& rhs) noexcept = default;

		const GlobalTexturePageInfo& GetDestinationGlobalTexturePageInfo() const;
		const GlobalTexturePageInfo& GetSourceGlobalTexturePageInfo() const;

	private:
		GlobalTexturePageInfo mDestPageInfo;
		GlobalTexturePageInfo mSrcPageInfo;
	};
}