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
			D3D12::TextureCopyRegion DestinationCopyRegion;
			D3D12::TextureCopyBufferSubAllocation SourceCopySubAllocation;
			std::uint32_t IndirectionTextureTexelValue;
		};

	public:
		IndirectionTextureUpdater() = default;

		IndirectionTextureUpdater(const IndirectionTextureUpdater& rhs) = delete;
		IndirectionTextureUpdater& operator=(const IndirectionTextureUpdater& rhs) = delete;

		IndirectionTextureUpdater(IndirectionTextureUpdater&& rhs) noexcept = default;
		IndirectionTextureUpdater& operator=(IndirectionTextureUpdater&& rhs) noexcept = default;

		void AddUpdatesForPageSwapOperation(const GlobalTexturePageSwapOperation& pageSwapOperation);
		void AllocateDataUploadBuffer(D3D12::FrameGraphBuilder& builder);

		void AddIndirectionTextureUpdateCommands(D3D12::DirectContext& context) const;

	private:
		void AddReplacementPageUpdateInfo(const GlobalTexturePageSwapOperation& pageSwapOperation);

	private:
		std::vector<UpdateInfo> mUpdateInfoArr;
		std::size_t mRequiredUploadBufferSize;
	};
}