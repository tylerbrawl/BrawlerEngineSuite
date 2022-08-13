module;
#include <vector>
#include <mutex>
#include <span>
#include <memory>

export module Brawler.GlobalTexturePageUploadSubModule;
import Brawler.GlobalTexturePageUploadSet;
import Brawler.IndirectionTextureUpdater;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.TextureCopyBufferSnapshot;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.FrameGraphBuilding;

export namespace Brawler
{
	class GlobalTexturePageUploadSubModule
	{
	private:
		struct PendingUploadBufferDeletion
		{
			std::unique_ptr<D3D12::BufferResource> UploadBufferPtr;
			std::uint64_t SafeDeletionFrameNumber;
		};

		struct GlobalTextureCopyPassInfo
		{
			D3D12::TextureCopyRegion DestCopyRegion;
			D3D12::TextureCopyBufferSnapshot SrcCopySnapshot;
		};

	public:
		using GlobalTextureCopyRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, GlobalTextureCopyPassInfo>;
		using IndirectionTextureUpdateRenderPass_T = IndirectionTextureUpdater::IndirectionTextureUpdateRenderPass_T;

		struct GlobalTextureUploadPassCollection
		{
			std::vector<GlobalTextureCopyRenderPass_T> GlobalTextureCopyPassArr;
			std::vector<IndirectionTextureUpdateRenderPass_T> IndirectionTextureUpdatePassArr;
		};

	public:
		GlobalTexturePageUploadSubModule() = default;

		GlobalTexturePageUploadSubModule(const GlobalTexturePageUploadSubModule& rhs) = delete;
		GlobalTexturePageUploadSubModule& operator=(const GlobalTexturePageUploadSubModule& rhs) = delete;

		GlobalTexturePageUploadSubModule(GlobalTexturePageUploadSubModule&& rhs) noexcept = default;
		GlobalTexturePageUploadSubModule& operator=(GlobalTexturePageUploadSubModule&& rhs) noexcept = default;

		void AddPageUploadRequests(GlobalTexturePageUploadSet&& uploadSet);
		bool HasPageUploadRequests() const;

		GlobalTextureUploadPassCollection GetPageUploadRenderPasses(D3D12::FrameGraphBuilder& builder);

	private:
		void CheckForUploadBufferDeletions();

		static std::vector<GlobalTextureCopyRenderPass_T> CreateGlobalTextureCopyRenderPasses(const std::span<const GlobalTexturePageUploadSet> uploadSetSpan);
		static std::vector<IndirectionTextureUpdateRenderPass_T> CreateIndirectionTextureUpdateRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const GlobalTexturePageUploadSet> uploadSetSpan);

	private:
		std::vector<GlobalTexturePageUploadSet> mUploadSetArr;
		mutable std::mutex mCritSection;
		std::vector<PendingUploadBufferDeletion> mPendingDeletionArr;
	};
}