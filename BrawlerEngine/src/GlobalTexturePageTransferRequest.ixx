module;

export module Brawler.GlobalTexturePageTransferRequest;
import Brawler.GlobalTexturePageInfo;

export namespace Brawler
{
	class GlobalTexturePageTransferRequest
	{
	public:
		GlobalTexturePageTransferRequest() = default;
		GlobalTexturePageTransferRequest(GlobalTexturePageInfo&& destPageInfo, GlobalTexturePageInfo&& srcPageInfo);

		GlobalTexturePageTransferRequest(const GlobalTexturePageTransferRequest& rhs) = delete;
		GlobalTexturePageTransferRequest& operator=(const GlobalTexturePageTransferRequest& rhs) = delete;

		GlobalTexturePageTransferRequest(GlobalTexturePageTransferRequest&& rhs) noexcept = default;
		GlobalTexturePageTransferRequest& operator=(GlobalTexturePageTransferRequest&& rhs) noexcept = default;

	private:
		GlobalTexturePageInfo mDestPageInfo;
		GlobalTexturePageInfo mSrcPageInfo;
	};
}