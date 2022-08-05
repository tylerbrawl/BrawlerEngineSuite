module;
#include <vector>
#include <unordered_map>
#include <memory>

export module Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTextureLogicalPage;
import Brawler.PolymorphicAdapter;
import Brawler.I_VirtualTexturePageState;
export import Brawler.VirtualTexturePageStateTraits;
import Brawler.GlobalTexturePageInfo;
import Brawler.GlobalTexturePageUploadRequest;
import Brawler.GlobalTexturePageTransferRequest;
import Brawler.GlobalTexturePageRemovalRequest;

export namespace Brawler
{
	class GlobalTextureUpdateContext
	{
	public:
		GlobalTextureUpdateContext() = default;

		GlobalTextureUpdateContext(const GlobalTextureUpdateContext& rhs) = delete;
		GlobalTextureUpdateContext& operator=(const GlobalTextureUpdateContext& rhs) = delete;

		GlobalTextureUpdateContext(GlobalTextureUpdateContext&& rhs) noexcept = default;
		GlobalTextureUpdateContext& operator=(GlobalTextureUpdateContext&& rhs) noexcept = default;

		void OnPageAddedToGlobalTexture(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& pageInfo);
		void OnPageRemovedFromGlobalTexture(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& pageInfo);

		void FinalizeContext();

	private:
		void FinalizeVirtualTexturePageStates();
		void CreatePageRequests();

	private:
		std::unordered_map<VirtualTextureLogicalPage, PolymorphicAdapter<I_VirtualTexturePageState>> mPageStateMap;
		std::vector<std::unique_ptr<GlobalTexturePageUploadRequest>> mUploadRequestPtrArr;
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> mTransferRequestPtrArr;
		std::vector<std::unique_ptr<GlobalTexturePageRemovalRequest>> mRemovalRequestPtrArr;
	};
}