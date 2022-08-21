module;
#include <vector>

export module Brawler.VirtualTextureManagementPassCollection;
import Brawler.GlobalTexturePageUploadSubModule;
import Brawler.GlobalTexturePageRemovalSubModule;
import Brawler.GlobalTexturePageTransferSubModule;
import Brawler.D3D12.RenderPassBundle;

export namespace Brawler
{
	class VirtualTextureManagementPassCollection
	{
	private:
		using UploadPassCollection_T = GlobalTexturePageUploadSubModule::GlobalTextureUploadPassCollection;
		using RemovalPass_T = GlobalTexturePageRemovalSubModule::IndirectionTextureUpdateRenderPass_T;
		using TransferPassCollection_T = GlobalTexturePageTransferSubModule::GlobalTextureTransferPassCollection;

	public:
		VirtualTextureManagementPassCollection() = default;

		VirtualTextureManagementPassCollection(const VirtualTextureManagementPassCollection& rhs) = delete;
		VirtualTextureManagementPassCollection& operator=(const VirtualTextureManagementPassCollection& rhs) = delete;

		VirtualTextureManagementPassCollection(VirtualTextureManagementPassCollection&& rhs) noexcept = default;
		VirtualTextureManagementPassCollection& operator=(VirtualTextureManagementPassCollection&& rhs) noexcept = default;

		void SetGlobalTextureUploadPasses(UploadPassCollection_T&& uploadPassCollection);
		void SetGlobalTextureRemovalPasses(std::vector<RemovalPass_T>&& removalPassArr);
		void SetGlobalTextureTransferPasses(TransferPassCollection_T&& transferPassCollection);

		void MoveRenderPassesIntoBundle(D3D12::RenderPassBundle& bundle);

	private:
		std::vector<GlobalTexturePageRemovalSubModule::IndirectionTextureUpdateRenderPass_T> mRemovalPassArr;
		GlobalTexturePageTransferSubModule::GlobalTextureTransferPassCollection mTransferPassCollection;
		GlobalTexturePageUploadSubModule::GlobalTextureUploadPassCollection mUploadPassCollection;
	};
}