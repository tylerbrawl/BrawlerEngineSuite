module;

export module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;

export namespace Brawler
{
	class GlobalTexturePageUploadRequest
	{
	public:
		GlobalTexturePageUploadRequest() = default;
		GlobalTexturePageUploadRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo);

		GlobalTexturePageUploadRequest(const GlobalTexturePageUploadRequest& rhs) = delete;
		GlobalTexturePageUploadRequest& operator=(const GlobalTexturePageUploadRequest& rhs) = delete;

		GlobalTexturePageUploadRequest(GlobalTexturePageUploadRequest&& rhs) noexcept = default;
		GlobalTexturePageUploadRequest& operator=(GlobalTexturePageUploadRequest&& rhs) noexcept = default;

	private:
		VirtualTextureLogicalPage mLogicalPage;
		GlobalTexturePageInfo mPageInfo;
	};
}