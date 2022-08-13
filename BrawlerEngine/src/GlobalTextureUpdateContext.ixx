module;
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

export module Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTextureLogicalPage;
import Brawler.PolymorphicAdapter;
import Brawler.I_VirtualTexturePageState;
export import Brawler.VirtualTexturePageStateTraits;
import Brawler.GlobalTexturePageInfo;
import Brawler.GlobalTexturePageUploadSet;
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

		bool HasPendingGlobalTextureChanges() const;

		/// <summary>
		/// Describes whether or not the changes which were recorded within this GlobalTextureUpdateContext
		/// instance are ready to be applied on the GPU. The GlobalTextureUpdateContext instance should
		/// *NOT* be submitted to the GPUSceneUpdateRenderModule until this function returns true!
		/// 
		/// If no changes were recorded into this GlobalTextureUpdateContext instance, then this function
		/// returns true. However, in that case, there is no need to submit the GlobalTextureUpdateContext
		/// instance to the GPUSceneUpdateRenderModule.
		/// </summary>
		/// <returns>
		/// The function returns true if this GlobalTextureUpdateContext instance can be submitted to the
		/// GPUSceneUpdateRenderModule and false otherwise. In the event that no changes were recorded into
		/// this GlobalTextureUpdateContext instance, the function will still return true, but submitting the
		/// GlobalTextureUpdateContext instance will ultimately have no effect.
		/// </returns>
		bool ReadyForGPUSubmission() const;

	private:
		void FinalizeVirtualTexturePageStates();
		void CreatePageRequests();

	private:
		std::unordered_map<VirtualTextureLogicalPage, PolymorphicAdapter<I_VirtualTexturePageState>> mPageStateMap;
		std::optional<GlobalTexturePageUploadSet> mPageUploadSet;
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> mTransferRequestPtrArr;
		std::vector<std::unique_ptr<GlobalTexturePageRemovalRequest>> mRemovalRequestPtrArr;
	};
}