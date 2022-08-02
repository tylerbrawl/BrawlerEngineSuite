module;
#include <vector>
#include <memory>
#include <span>

export module Brawler.VirtualTextureManagementSubModule:GlobalTextureUpdater;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.GlobalTextureUploadBuffer;

export namespace Brawler
{
	class GlobalTextureUpdater
	{
	private:
		struct GlobalTextureUpdateInfo
		{
			D3D12::TextureCopyRegion DestinationCopyRegion;
			D3D12::TextureCopyBufferSnapshot SourceCopyBufferSnapshot;
		};

		struct GlobalTextureUpdatePassInfo
		{
			std::vector<GlobalTextureUpdateInfo> UpdateInfoArr;
		};

	public:
		using GlobalTextureUpdatePass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, GlobalTextureUpdatePassInfo>;

	public:
		GlobalTextureUpdater() = default;

		GlobalTextureUpdater(const GlobalTextureUpdater& rhs) = delete;
		GlobalTextureUpdater& operator=(const GlobalTextureUpdater& rhs) = delete;

		GlobalTextureUpdater(GlobalTextureUpdater&& rhs) noexcept = default;
		GlobalTextureUpdater& operator=(GlobalTextureUpdater&& rhs) noexcept = default;

		GlobalTextureUpdatePass_T CreateGlobalTextureUpdatesRenderPass(const std::span<const std::unique_ptr<GlobalTextureUploadBuffer>> uploadBufferSpan) const;
	};
}