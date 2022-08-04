module;
#include <tuple>

export module Brawler.VirtualTexturePageStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.I_VirtualTexturePageState;
import Brawler.VirtualTexturePageStates;
import Brawler.VirtualTexturePageStateID;

namespace Brawler
{
	using StateTupleType = std::tuple<
		VirtualTexturePageNeutralState,				// VirtualTexturePageStateID::PAGE_NEUTRAL_STATE
		VirtualTexturePageAdditionState,			// VirtualTexturePageStateID::PAGE_ADDITION_STATE
		VirtualTexturePagePendingRemovalState,		// VirtualTexturePageStateID::PAGE_PENDING_REMOVAL_STATE
		VirtualTexturePageCommittedRemovalState,	// VirtualTexturePageStateID::PAGE_COMMITTED_REMOVAL_STATE
		VirtualTexturePageTransferState				// VirtualTexturePageStateID::PAGE_TRANSFER_STATE
	>;
}

export namespace Brawler
{
	template <typename DummyType>
	struct PolymorphismInfo<I_VirtualTexturePageState<DummyType>> : public PolymorphismInfoInstantiation<VirtualTexturePageStateID, StateTupleType>
	{};
}