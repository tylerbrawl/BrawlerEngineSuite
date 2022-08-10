module;
#include <mutex>
#include <vector>
#include <span>
#include <memory>

export module Brawler.GlobalTexturePageTransferSubModule;
import Brawler.GlobalTexturePageTransferRequest;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.IndirectionTextureUpdater;

export namespace Brawler
{
	class GlobalTexturePageTransferSubModule
	{
	private:
		struct GlobalTextureCopyPassInfo
		{
			D3D12::TextureCopyRegion DestCopyRegion;
			D3D12::TextureCopyRegion SrcCopyRegion;
		};

	public:
		using GlobalTextureCopyRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, GlobalTextureCopyPassInfo>;
		using IndirectionTextureUpdateRenderPass_T = IndirectionTextureUpdater::IndirectionTextureUpdateRenderPass_T;

		struct GlobalTextureTransferPassCollection
		{
			std::vector<GlobalTextureCopyRenderPass_T> GlobalTextureCopyArr;
			std::vector<IndirectionTextureUpdateRenderPass_T> IndirectionTextureUpdateArr;
		};

	public:
		GlobalTexturePageTransferSubModule() = default;

		GlobalTexturePageTransferSubModule(const GlobalTexturePageTransferSubModule& rhs) = delete;
		GlobalTexturePageTransferSubModule& operator=(const GlobalTexturePageTransferSubModule& rhs) = delete;

		GlobalTexturePageTransferSubModule(GlobalTexturePageTransferSubModule&& rhs) noexcept = default;
		GlobalTexturePageTransferSubModule& operator=(GlobalTexturePageTransferSubModule&& rhs) noexcept = default;

		void AddPageTransferRequests(const std::span<std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan);
		bool HasPageTransferRequests() const;

		GlobalTextureTransferPassCollection GetPageTransferRenderPasses(D3D12::FrameGraphBuilder& builder);

	private:
		static std::vector<GlobalTextureCopyRenderPass_T> CreateGlobalTextureCopyRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan);
		static std::vector<IndirectionTextureUpdateRenderPass_T> CreateIndirectionTextureUpdateRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan);

	private:
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> mTransferRequestPtrArr;
		mutable std::mutex mCritSection;
	};
}