module;
#include <unordered_map>
#include <optional>

module Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTexturePageStates;

namespace Brawler
{
	void GlobalTextureUpdateContext::OnPageAddedToGlobalTexture(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& pageInfo)
	{
		if (!mPageStateMap.contains(logicalPage))
			mPageStateMap[logicalPage] = VirtualTexturePageNeutralState{};

		// I Can't Believe This Builds (TM)!
		std::optional<PolymorphicAdapter<I_VirtualTexturePageState>> optionalNewStateAdapter{};
		mPageStateMap[logicalPage].AccessData([&optionalNewStateAdapter, &pageInfo]<typename StateType>(StateType & state)
		{
			auto optionalNewState{ state.OnPageAddedToGlobalTexture(std::move(pageInfo)) };

			if (optionalNewState.has_value()) [[likely]]
				optionalNewStateAdapter = std::move(*optionalNewState);
		});

		if (optionalNewStateAdapter.has_value()) [[likely]]
			mPageStateMap[logicalPage] = std::move(*optionalNewStateAdapter);
	}

	void GlobalTextureUpdateContext::OnPageRemovedFromGlobalTexture(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& pageInfo)
	{
		if (!mPageStateMap.contains(logicalPage))
			mPageStateMap[logicalPage] = VirtualTexturePageNeutralState{};

		std::optional<PolymorphicAdapter<I_VirtualTexturePageState>> optionalNewStateAdapter{};
		mPageStateMap[logicalPage].AccessData([&pageInfo, &optionalNewStateAdapter]<typename StateType>(StateType & state)
		{
			auto optionalNewState{ state.OnPageRemovedFromGlobalTexture(std::move(pageInfo)) };

			if (optionalNewState.has_value()) [[likely]]
				optionalNewStateAdapter = std::move(*optionalNewState);
		});

		if (optionalNewStateAdapter.has_value()) [[likely]]
			mPageStateMap[logicalPage] = std::move(*optionalNewStateAdapter);
	}

	void GlobalTextureUpdateContext::FinalizeContext()
	{
		FinalizeVirtualTexturePageStates();
	}

	void GlobalTextureUpdateContext::FinalizeVirtualTexturePageStates()
	{
		for (const auto& [logicalPage, stateAdapter] : mPageStateMap)
		{
			std::optional<PolymorphicAdapter<I_VirtualTexturePageState>> optionalFinalStateAdapter{};
			mPageStateMap[logicalPage].AccessData([&optionalFinalStateAdapter]<typename StateType>(StateType & state)
			{
				auto optionalFinalState{ state.OnContextFinalization() };

				if (optionalFinalState.has_value())
					optionalFinalStateAdapter = std::move(*optionalFinalState);
			});

			if (optionalFinalStateAdapter.has_value())
				stateAdapter = std::move(*optionalFinalStateAdapter);
		}
	}
}