module;
#include <cstdint>
#include <vector>

export module Brawler.VirtualTextureManagementSubModule:IndirectionTextureUpdater;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.GlobalTexturePageSwapOperation;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.DirectContext;

export namespace Brawler
{
	class IndirectionTextureUpdater
	{
	private:
		struct UpdateInfo
		{
			// We need to store the D3D12::Texture2DSubResource separately because
			// D3D12::TextureCopyRegion can only return a const I_GPUResource&.
			D3D12::Texture2DSubResource DestinationSubResource;
			
			D3D12::TextureCopyRegion DestinationCopyRegion;
			D3D12::TextureCopyBufferSubAllocation SourceCopySubAllocation;
			std::uint32_t IndirectionTextureTexelValue;
		};

		struct IndirectionTextureUpdatePassInfo
		{
			std::vector<UpdateInfo> UpdateInfoArr;
		};

	public:
		using IndirectionTextureUpdatePass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, IndirectionTextureUpdatePassInfo>;

	public:
		IndirectionTextureUpdater() = default;

		IndirectionTextureUpdater(const IndirectionTextureUpdater& rhs) = delete;
		IndirectionTextureUpdater& operator=(const IndirectionTextureUpdater& rhs) = delete;

		IndirectionTextureUpdater(IndirectionTextureUpdater&& rhs) noexcept = default;
		IndirectionTextureUpdater& operator=(IndirectionTextureUpdater&& rhs) noexcept = default;

		void AddUpdatesForPageSwapOperation(const GlobalTexturePageSwapOperation& pageSwapOperation);
		IndirectionTextureUpdatePass_T CreateIndirectionTextureUpdatesRenderPass(D3D12::FrameGraphBuilder& builder);

	private:
		void AddReplacementPageUpdateInfo(const GlobalTexturePageSwapOperation& pageSwapOperation);
		void AddOldPageUpdateInfo(const GlobalTexturePageSwapOperation& pageSwapOperation);

		void AllocateDataUploadBuffer(D3D12::FrameGraphBuilder& builder);

	private:
		std::vector<UpdateInfo> mUpdateInfoArr;
		std::size_t mRequiredUploadBufferSize;
	};
}