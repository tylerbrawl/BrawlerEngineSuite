module;

export module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;
import Brawler.D3D12.TextureCopyBufferSubAllocation;

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

		const VirtualTextureLogicalPage& GetLogicalPage() const;
		const GlobalTexturePageInfo& GetDestinationGlobalTexturePageInfo() const;

		D3D12::TextureCopyBufferSubAllocation& GetPageDataBufferSubAllocation() const;

	private:
		VirtualTextureLogicalPage mLogicalPage;
		GlobalTexturePageInfo mPageInfo;
		D3D12::TextureCopyBufferSubAllocation mPageDataCopySubAllocation;
	};
}