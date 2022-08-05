module;
#include <mutex>
#include <vector>
#include <span>
#include <memory>

export module Brawler.VirtualTextureManagementSubModule:GlobalTexturePageTransferSubModule;
import Brawler.GlobalTexturePageTransferRequest;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.TextureCopyBufferSnapshot;
import Brawler.D3D12.TextureCopyRegion;

export namespace Brawler
{
	class GlobalTexturePageTransferSubModule
	{
	private:
		enum class PageCopyType
		{
			COPY_TO_BUFFER,
			COPY_TO_GLOBAL_TEXTURE
		};

		struct PageTransferPassInfo
		{
			D3D12::TextureCopyRegion TextureRegion;
			D3D12::TextureCopyBufferSnapshot BufferRegion;

			PageCopyType PassType;
		};

	public:
		using PageTransferRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, PageTransferPassInfo>;

	public:
		GlobalTexturePageTransferSubModule() = default;

		GlobalTexturePageTransferSubModule(const GlobalTexturePageTransferSubModule& rhs) = delete;
		GlobalTexturePageTransferSubModule& operator=(const GlobalTexturePageTransferSubModule& rhs) = delete;

		GlobalTexturePageTransferSubModule(GlobalTexturePageTransferSubModule&& rhs) noexcept = default;
		GlobalTexturePageTransferSubModule& operator=(GlobalTexturePageTransferSubModule&& rhs) noexcept = default;

		void AddPageTransferRequests(const std::span<std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan);
		bool HasPageTransferRequests() const;

		std::vector<PageTransferRenderPass_T> GetPageTransferRenderPasses(D3D12::FrameGraphBuilder& builder);

	private:
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> mTransferRequestPtrArr;
		mutable std::mutex mCritSection;
	};
}