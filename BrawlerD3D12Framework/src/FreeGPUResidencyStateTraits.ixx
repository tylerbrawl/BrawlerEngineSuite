module;
#include <tuple>

export module Brawler.FreeGPUResidencyStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.D3D12.I_FreeGPUResidencyState;
import Brawler.D3D12.EvictPageableGPUObjectState;
import Brawler.D3D12.DeletePageableGPUObjectState;
import Brawler.D3D12.FreeGPUResidencyStateID;

namespace Brawler
{
	namespace D3D12
	{
		template <FreeGPUResidencyStateID ID>
		struct StateMap
		{
			static_assert(sizeof(ID) != sizeof(ID));
		};

		template <typename StateType_>
		struct StateMapInstantiation
		{
			using StateType = StateType_;
		};

		template <>
		struct StateMap<FreeGPUResidencyStateID::EVICT_PAGEABLE_GPU_OBJECT> : public StateMapInstantiation<EvictPageableGPUObjectState>
		{};

		template <>
		struct StateMap<FreeGPUResidencyStateID::DELETE_PAGEABLE_GPU_OBJECT> : public StateMapInstantiation<DeletePageableGPUObjectState>
		{};

		template <FreeGPUResidencyStateID CurrID>
		consteval auto GetStateTypeTuple()
		{
			constexpr FreeGPUResidencyStateID NEXT_ID = static_cast<FreeGPUResidencyStateID>(std::to_underlying(CurrID) + 1);

			if constexpr (NEXT_ID != FreeGPUResidencyStateID::COUNT_OR_ERROR)
				return std::tuple_cat(std::tuple<typename StateMap<CurrID>::StateType>{}, GetStateTypeTuple<NEXT_ID>());
			else
				return std::tuple<typename StateMap<CurrID>::StateType>{};
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	template <typename DummyType>
	struct PolymorphismInfo<D3D12::I_FreeGPUResidencyState<DummyType>> : public PolymorphismInfoInstantiation<D3D12::FreeGPUResidencyStateID, decltype(D3D12::GetStateTypeTuple<static_cast<D3D12::FreeGPUResidencyStateID>(0)>())>
	{};
}