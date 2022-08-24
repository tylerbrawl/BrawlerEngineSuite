module;

export module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTextureStreamingNotifier;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;
import Brawler.D3D12.TextureCopyBufferSubAllocation;

export namespace Brawler
{
	class GlobalTexturePageUploadRequest final : public VirtualTextureStreamingNotifier
	{
	public:
		GlobalTexturePageUploadRequest() = default;
		GlobalTexturePageUploadRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo);

		~GlobalTexturePageUploadRequest();

		GlobalTexturePageUploadRequest(const GlobalTexturePageUploadRequest& rhs) = delete;
		GlobalTexturePageUploadRequest& operator=(const GlobalTexturePageUploadRequest& rhs) = delete;

		GlobalTexturePageUploadRequest(GlobalTexturePageUploadRequest&& rhs) noexcept = default;
		GlobalTexturePageUploadRequest& operator=(GlobalTexturePageUploadRequest&& rhs) noexcept = default;

		const GlobalTexturePageInfo& GetDestinationGlobalTexturePageInfo() const;

		D3D12::TextureCopyBufferSubAllocation& GetPageDataBufferSubAllocation();
		const D3D12::TextureCopyBufferSubAllocation& GetPageDataBufferSubAllocation() const;

	private:
		VirtualTextureLogicalPage mLogicalPage;
		GlobalTexturePageInfo mPageInfo;
		D3D12::TextureCopyBufferSubAllocation mPageDataCopySubAllocation;
	};
}