module;
#include <vector>
#include <memory>
#include <optional>
#include <span>
#include <DxDef.h>

export module Brawler.GlobalTexturePageUploadSet;
import Brawler.GlobalTexturePageUploadRequest;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.AlignedByteAddressBufferSubAllocation;
import Brawler.AssetManagement.AssetRequestEventHandle;

export namespace Brawler
{
	class GlobalTexturePageUploadSet
	{
	public:
		GlobalTexturePageUploadSet() = default;

		GlobalTexturePageUploadSet(const GlobalTexturePageUploadSet& rhs) = delete;
		GlobalTexturePageUploadSet& operator=(const GlobalTexturePageUploadSet& rhs) = delete;

		GlobalTexturePageUploadSet(GlobalTexturePageUploadSet&& rhs) noexcept = default;
		GlobalTexturePageUploadSet& operator=(GlobalTexturePageUploadSet&& rhs) noexcept = default;

		void AddPageUploadRequest(std::unique_ptr<GlobalTexturePageUploadRequest>&& uploadRequestPtr);
		void PrepareRequestedPageData();

		bool HasActiveUploadRequests() const;
		bool ReadyForGlobalTextureUploads() const;

		std::span<const std::unique_ptr<GlobalTexturePageUploadRequest>> GetUploadRequestSpan() const;

	private:
		std::unique_ptr<D3D12::BufferResource> mUploadBufferPtr;
		std::vector<std::unique_ptr<GlobalTexturePageUploadRequest>> mRequestPtrArr;
		AssetManagement::AssetRequestEventHandle mHPageDataUploadEvent;
	};
}