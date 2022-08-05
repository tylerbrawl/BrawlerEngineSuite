module;
#include <unordered_map>

export module Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTextureLogicalPage;
import Brawler.PolymorphicAdapter;
import Brawler.I_VirtualTexturePageState;
export import Brawler.VirtualTexturePageStateTraits;
import Brawler.GlobalTexturePageInfo;

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

	private:
		std::unordered_map<VirtualTextureLogicalPage, PolymorphicAdapter<I_VirtualTexturePageState>> mPageStateMap;
	};
}