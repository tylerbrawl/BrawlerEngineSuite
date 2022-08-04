module;
#include <unordered_map>

export module Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTextureLogicalPage;
import Brawler.PolymorphicAdapter;
import Brawler.I_VirtualTexturePageState;
export import Brawler.VirtualTexturePageStateTraits;

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

	private:
		std::unordered_map<VirtualTextureLogicalPage, PolymorphicAdapter<I_VirtualTexturePageState>> mPageStateMap;
	};
}