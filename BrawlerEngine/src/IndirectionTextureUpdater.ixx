module;
#include <cstdint>
#include <array>
#include <vector>

export module Brawler.IndirectionTextureUpdater;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;
import Brawler.FrameGraphBuilding;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.Texture2D;

export namespace Brawler
{
	class IndirectionTextureUpdater
	{
	private:
		struct IndirectionTextureUpdatePassInfo
		{
			D3D12::TextureCopyRegion DestCopyRegion;
			D3D12::TextureCopyBufferSubAllocation SrcSubAllocation;
		};

		struct UpdateInfoStorage
		{
			IndirectionTextureUpdatePassInfo PassInfo;
			D3D12::Texture2DSubResource IndirectionTextureSubResource;
			std::array<std::uint32_t, 2> IndirectionTexelValueArr;
		};

	public:
		using IndirectionTextureUpdateRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, IndirectionTextureUpdatePassInfo>;

	public:
		IndirectionTextureUpdater() = default;

		IndirectionTextureUpdater(const IndirectionTextureUpdater& rhs) = delete;
		IndirectionTextureUpdater& operator=(const IndirectionTextureUpdater& rhs) = delete;

		IndirectionTextureUpdater(IndirectionTextureUpdater&& rhs) noexcept = default;
		IndirectionTextureUpdater& operator=(IndirectionTextureUpdater&& rhs) noexcept = default;

		void UpdateIndirectionTextureEntry(const VirtualTextureLogicalPage& logicalPage, const GlobalTexturePageInfo& destinationPageInfo);
		void ClearIndirectionTextureEntry(const VirtualTextureLogicalPage& logicalPage);

		std::vector<IndirectionTextureUpdateRenderPass_T> FinalizeIndirectionTextureUpdates(D3D12::FrameGraphBuilder& builder);

	private:
		void AddUpdateInfo(const VirtualTextureLogicalPage& logicalPage, const std::uint32_t newIndirectionTexelValue);

	private:
		std::vector<UpdateInfoStorage> mUpdateInfoStorageArr;
		std::size_t mCurrRequiredBufferSize;
	};
}