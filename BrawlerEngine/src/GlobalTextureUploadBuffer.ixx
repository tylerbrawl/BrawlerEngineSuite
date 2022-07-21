module;
#include <memory>
#include <vector>

export module Brawler.GlobalTextureUploadBuffer;
import Brawler.D3D12.BufferResource;
import Brawler.GlobalTexturePageSwapOperation;
import Brawler.AssetManagement.AssetRequestEventHandle;

export namespace Brawler
{
	class GlobalTextureUploadBuffer
	{
	public:
		GlobalTextureUploadBuffer() = default;

		GlobalTextureUploadBuffer(const GlobalTextureUploadBuffer& rhs) = delete;
		GlobalTextureUploadBuffer& operator=(const GlobalTextureUploadBuffer& rhs) = delete;

		GlobalTextureUploadBuffer(GlobalTextureUploadBuffer&& rhs) noexcept = default;
		GlobalTextureUploadBuffer& operator=(GlobalTextureUploadBuffer&& rhs) noexcept = default;

		void AddPageSwapOperation(std::unique_ptr<GlobalTexturePageSwapOperation>&& pageSwapOperation);
		void BeginTextureDataStreaming();

		bool IsTextureDataPrepared() const;

	private:
		std::unique_ptr<D3D12::BufferResource> mUploadBufferPtr;
		std::vector<std::unique_ptr<GlobalTexturePageSwapOperation>> mPageSwapOperationPtrArr;
		AssetManagement::AssetRequestEventHandle mHPageDataLoadEvent;
	};
}