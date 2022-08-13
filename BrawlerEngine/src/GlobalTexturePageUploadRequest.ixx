module;
#include <DxDef.h>

export module Brawler.GlobalTexturePageUploadRequest;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;
import Brawler.D3D12.DynamicAlignedByteAddressBufferSubAllocation;

export namespace Brawler
{
	class GlobalTexturePageUploadRequest
	{
	public:
		using PageDataSubAllocation = D3D12::DynamicAlignedByteAddressBufferSubAllocation<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>;

	public:
		GlobalTexturePageUploadRequest() = default;
		GlobalTexturePageUploadRequest(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& destPageInfo);

		GlobalTexturePageUploadRequest(const GlobalTexturePageUploadRequest& rhs) = delete;
		GlobalTexturePageUploadRequest& operator=(const GlobalTexturePageUploadRequest& rhs) = delete;

		GlobalTexturePageUploadRequest(GlobalTexturePageUploadRequest&& rhs) noexcept = default;
		GlobalTexturePageUploadRequest& operator=(GlobalTexturePageUploadRequest&& rhs) noexcept = default;

		const VirtualTextureLogicalPage& GetLogicalPage() const;
		const GlobalTexturePageInfo& GetDestinationGlobalTexturePageInfo() const;

		PageDataSubAllocation& GetPageDataBufferSubAllocation() const;

	private:
		VirtualTextureLogicalPage mLogicalPage;
		GlobalTexturePageInfo mPageInfo;
		PageDataSubAllocation mTextureDataSubAllocation;
	};
}