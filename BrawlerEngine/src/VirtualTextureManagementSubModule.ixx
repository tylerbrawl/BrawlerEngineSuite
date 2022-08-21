module;
#include <memory>
#include <span>
#include <vector>
#include <mutex>

export module Brawler.VirtualTextureManagementSubModule;
import Brawler.GlobalTexturePageUploadSubModule;
import Brawler.GlobalTexturePageRemovalSubModule;
import Brawler.GlobalTexturePageTransferSubModule;
import Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTextureManagementPassCollection;
import Brawler.D3D12.FrameGraphBuilding;

export namespace Brawler
{
	class VirtualTextureManagementSubModule
	{
	public:
		VirtualTextureManagementSubModule() = default;

		VirtualTextureManagementSubModule(const VirtualTextureManagementSubModule& rhs) = delete;
		VirtualTextureManagementSubModule& operator=(const VirtualTextureManagementSubModule& rhs) = delete;

		VirtualTextureManagementSubModule(VirtualTextureManagementSubModule&& rhs) noexcept = default;
		VirtualTextureManagementSubModule& operator=(VirtualTextureManagementSubModule&& rhs) noexcept = default;

		void CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUpdateContext>&& updateContextPtr);
		bool HasCommittedGlobalTextureChanges() const;

		VirtualTextureManagementPassCollection CreateGlobalTextureChangeRenderPasses(D3D12::FrameGraphBuilder& builder);

	private:
		GlobalTexturePageUploadSubModule mPageUploadSubModule;
		GlobalTexturePageRemovalSubModule mPageRemovalSubModule;
		GlobalTexturePageTransferSubModule mPageTransferSubModule;
		std::vector<std::unique_ptr<GlobalTextureUpdateContext>> mPendingContextPtrArr;
		mutable std::mutex mCritSection;
	};
}