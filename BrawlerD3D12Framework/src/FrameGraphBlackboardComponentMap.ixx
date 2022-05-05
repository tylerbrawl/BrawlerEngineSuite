module;
#include <tuple>
#include <variant>

export module Brawler.D3D12.FrameGraphBlackboardComponentMap;
import Brawler.D3D12.FrameGraphBlackboardComponent;

namespace Brawler
{
	namespace D3D12
	{
		template <FrameGraphBlackboardComponent Component>
		struct FrameGraphBlackboardComponentMap
		{
			static_assert(sizeof(Component) != sizeof(Component), "ERROR: An explicit template specialization of Brawler::D3D12::FrameGraphBlackboardComponentMap was never specified for this Brawler::D3D12::FrameGraphBlackboardComponent! (See FrameGraphBlackboardComponentMap.ixx.)");
		};

		template <typename T>
		concept IsVariantEligible = !std::is_reference_v<T> && !std::is_array_v<T> && !std::is_same_v<T, void>;

		template <typename ComponentType_>
			requires IsVariantEligible<ComponentType_> && !std::is_pointer_v<ComponentType_>
		struct FrameGraphBlackboardComponentMapInstantiation
		{
			// The restrictions placed on ComponentType_ are those which are placed on all types which can
			// be held in a std::variant instance, in addition to the restriction that it cannot be a pointer
			// type.
			//
			// Resources which should be shared between I_RenderModule instances *MUST* be stored within the
			// blackboard. Otherwise, it becomes possible/easy to accidentally "update" shared data locally,
			// only to find that the update was never actually made visible to all other modules.
			using ComponentType = ComponentType_;
		};

		// =======================================================================================================
		// Base Templates ^ | v Template Instantiations
		// =======================================================================================================

		template <>
		struct FrameGraphBlackboardComponentMap<FrameGraphBlackboardComponent::TEST_COMPONENT> : public FrameGraphBlackboardComponentMapInstantiation<std::int32_t>
		{};

		// =======================================================================================================
		// Template Instantiations ^ | v Implementation Details
		// =======================================================================================================

		template <FrameGraphBlackboardComponent CurrComponent>
		consteval auto CreateComponentTupleIMPL()
		{
			typename FrameGraphBlackboardComponentMap<CurrComponent>::ComponentType element{};
			
			if constexpr ((std::to_underlying(CurrComponent) + 1) != std::to_underlying(FrameGraphBlackboardComponent::COUNT_OR_ERROR))
				return std::tuple_cat(std::tie(element), CreateComponentTupleIMPL<static_cast<FrameGraphBlackboardComponent>(std::to_underlying(CurrComponent) + 1)>());
			
			return std::tie(element);
		}

		consteval auto CreateComponentTuple()
		{
			return CreateComponentTupleIMPL<static_cast<FrameGraphBlackboardComponent>(0)>();
		}

		template <typename T>
		struct TupleTypeSolver
		{};

		template <typename... T>
		struct TupleTypeSolver<std::tuple<T...>>
		{
			// We add std::monostate as the first element to ensure that the variant type is
			// default constructible.
			using VariantType = std::variant<std::monostate, std::remove_reference_t<T>...>;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <FrameGraphBlackboardComponent Component>
		using FrameGraphBlackboardComponentType = FrameGraphBlackboardComponentMap<Component>::ComponentType;

		using FrameGraphBlackboardComponentVariantType = TupleTypeSolver<decltype(CreateComponentTuple())>::VariantType;
	}
}