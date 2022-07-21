module;
#include <memory>
#include <DxDef.h>

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

	private:
		std::unique_ptr<D3D12::BufferResource> mUploadBufferPtr;
		std::vector<std::unique_ptr<GlobalTexturePageSwapOperation>> mPageSwapOperationArr;
		AssetManagement::AssetRequestEventHandle mHPageDataLoadEvent;
	};
}