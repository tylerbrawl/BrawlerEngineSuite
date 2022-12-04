module;
#include <tuple>
#include <utility>

export module Brawler.FrameGraphBlackboard:FrameGraphBlackboardElementMap;
import Brawler.FrameGraphBlackboardElementID;

namespace Brawler
{
	template <FrameGraphBlackboardElementID ElementID>
	struct BlackboardElementIDMap
	{
		static_assert(sizeof(ElementID) != sizeof(ElementID), "ERROR: An explicit template specialization of Brawler::BlackboardElementIDMap was never provided for a given Brawler::FrameGraphBlackboardElementID value! (See FrameGraphBlackboardElementMap.ixx.)");
	};

	template <typename ElementType_>
	struct BlackboardElementIDMapInstantiation
	{
		using ElementType = ElementType_;
	};
}

namespace Brawler
{
	template <std::underlying_type_t<FrameGraphBlackboardElementID> CurrIndex, typename... OtherTypes>
	struct BlackboardElementTupleSolver
	{
	private:
		static constexpr FrameGraphBlackboardElementID CURR_ELEMENT_ID = static_cast<FrameGraphBlackboardElementID>(CurrIndex);
		using CurrElement_T = typename BlackboardElementIDMap<CURR_ELEMENT_ID>::ElementType;

	public:
		using ResultType = typename BlackboardElementTupleSolver<(CurrIndex - 1), CurrElement_T, OtherTypes...>::ResultType;
	};

	template <>
	struct BlackboardElementTupleSolver<std::to_underlying(FrameGraphBlackboardElementID::COUNT_OR_ERROR)>
	{
		using ResultType = typename BlackboardElementTupleSolver<(std::to_underlying(FrameGraphBlackboardElementID::COUNT_OR_ERROR) - 1)>::ResultType;
	};

	template <typename... OtherTypes>
	struct BlackboardElementTupleSolver<0, OtherTypes...>
	{
	private:
		static constexpr FrameGraphBlackboardElementID CURR_ELEMENT_ID = static_cast<FrameGraphBlackboardElementID>(0);
		using CurrElement_T = typename BlackboardElementIDMap<CURR_ELEMENT_ID>::ElementType;

	public:
		using ResultType = std::tuple<CurrElement_T, OtherTypes...>;
	};
}

export namespace Brawler
{
	using BlackboardElementTuple_T = typename BlackboardElementTupleSolver<std::to_underlying(FrameGraphBlackboardID::COUNT_OR_ERROR)>::ResultType;
}