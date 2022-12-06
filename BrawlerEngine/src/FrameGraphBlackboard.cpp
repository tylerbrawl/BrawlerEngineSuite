module;
#include <array>
#include <concepts>
#include <tuple>
#include <vector>
#include <memory>
#include <span>

module Brawler.FrameGraphBlackboard;
import Brawler.JobSystem;
import Brawler.BlackboardTransientResourceBuilder;
import Brawler.Application;
import Brawler.D3D12.Renderer;

namespace
{
	template <typename T>
	concept HasTransientResourcesToBuild = requires (T& blackboardElement, Brawler::BlackboardTransientResourceBuilder& builder)
	{
		{ blackboardElement.InitializeTransientResources(builder); } -> std::same_as<void>;
	};

	static constexpr std::size_t TRANSIENT_RESOURCE_BUILDER_COUNT = [] ()
	{
		std::size_t numElements = 0;
		const auto countElementsLambda = [&numElements]<std::size_t CurrIndex>(this const auto& self)
		{
			if constexpr (CurrIndex != std::to_underlying(Brawler::FrameGraphBlackboardElementID::COUNT_OR_ERROR))
			{
				using CurrElement_T = std::tuple_element_t<CurrIndex, Brawler::BlackboardElementTuple_T>;

				if constexpr (HasTransientResourcesToBuild<CurrElement_T>)
					++numElements;

				constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);
				self.template operator()<NEXT_INDEX>();
			}
		};

		countElementsLambda.template operator()<0>();

		return numElements;
	}();

	template <Brawler::FrameGraphBlackboardElementID ElementID>
		requires HasTransientResourcesToBuild<Brawler::BlackboardElement_T<ElementID>>
	consteval std::size_t GetTransientResourceBuilderIndex()
	{
		std::size_t currBuilderIndex = 0;
		const auto getBuilderIndexLambda = [&currBuilderIndex]<std::size_t CurrElementIndex>(this const auto& self)
		{
			// Do a linear search through all of the types in the BlackboardElementTuple_T tuple
			// type. For each type we find which satisfies the HasTransientResourcesToBuild concept,
			// we check if its corresponding index in the tuple matches the underlying value of
			// ElementID. If this is the case, then currBuilderIndex contains the return value;
			// otherwise, we increment currBuilderIndex and move on to the next type which satisfies
			// the concept.

			static_assert(CurrElementIndex != std::to_underlying(Brawler::FrameGraphBlackboardElementID::COUNT_OR_ERROR));

			using CurrElement_T = std::tuple_element_t<CurrElementIndex, Brawler::BlackboardElementTuple_T>;

			if constexpr (HasTransientResourcesToBuild<CurrElement_T>)
			{
				if constexpr (CurrElementIndex == std::to_underlying(ElementID))
					return currBuilderIndex;
				else
					++currBuilderIndex;
			}
			
			constexpr std::size_t NEXT_INDEX = (CurrElementIndex + 1);
			return self.template operator()<NEXT_INDEX>();
		};

		return getBuilderIndexLambda.template operator()<0>();
	}
}

namespace Brawler
{
	FrameGraphBlackboard::FrameGraphBlackboard() :
		mElementTupleArr(),
		mTransientResourceContainerArr()
	{
		// Add a persistent FrameGraph completion callback function for deleting old transient
		// resources and creating new ones.
		if constexpr (TRANSIENT_RESOURCE_BUILDER_COUNT > 0)
		{
			Brawler::GetRenderer().AddPersistentFrameGraphCompletionCallback([] ()
			{
				FrameGraphBlackboard::GetInstance().InitializeTransientResources();
			});
		}
	}

	FrameGraphBlackboard& FrameGraphBlackboard::GetInstance()
	{
		static FrameGraphBlackboard instance{};
		return instance;
	}

	void FrameGraphBlackboard::InitializeTransientResources()
	{
		if constexpr (TRANSIENT_RESOURCE_BUILDER_COUNT > 0)
		{
			std::array<BlackboardTransientResourceBuilder, TRANSIENT_RESOURCE_BUILDER_COUNT> transientResourceBuilderArr{};

			Brawler::JobGroup buildTransientResourcesGroup{};
			buildTransientResourcesGroup.Reserve(TRANSIENT_RESOURCE_BUILDER_COUNT);

			// Add a CPU job to buildTransientResourcesGroup which initializes the transient resources
			// for each element in the FrameGraphBlackboard which needs it. This search is done using
			// templated recursion, so the compiler should be able to unroll it and directly add the
			// jobs.

			const auto addBuildJobsLambda = [this, &buildTransientResourcesGroup, &transientResourceBuilderArr]<std::size_t CurrIndex>(this const auto& self)
			{
				static constexpr FrameGraphBlackboardElementID CURR_ELEMENT_ID = static_cast<FrameGraphBlackboardElementID>(CURR_ELEMENT_ID);

				if constexpr (CurrIndex != std::to_underlying(FrameGraphBlackboardElementID::COUNT_OR_ERROR))
				{
					using CurrElement_T = std::tuple_element_t<CurrIndex, BlackboardElementTuple_T>;

					if constexpr (HasTransientResourcesToBuild<CurrElement_T>)
					{
						static constexpr std::size_t CURR_TRANSIENT_RESOURCE_BUILDER_INDEX = GetTransientResourceBuilderIndex<CURR_ELEMENT_ID>();

						CurrElement_T& currBlackboardElement{ std::get<CurrIndex>(GetCurrentBlackboardElementTuple()) };
						BlackboardTransientResourceBuilder& currResourceBuilder{ transientResourceBuilderArr[CURR_TRANSIENT_RESOURCE_BUILDER_INDEX] };

						buildTransientResourcesGroup.AddJob([&currBlackboardElement, &currResourceBuilder] ()
						{
							currBlackboardElement.InitializeTransientResources(currResourceBuilder);
						});
					}

					static constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);
					self.template operator()<NEXT_INDEX>();
				}
			};

			addBuildJobsLambda.template operator()<0>();

			buildTransientResourcesGroup.ExecuteJobs();

			// For the current FrameGraph, delete the old transient resources and store the
			// new ones.
			TransientResourceContainer currFrameTransientResourceArr{};

			{
				std::size_t numTransientResources = 0;

				for (const auto& builder : transientResourceBuilderArr)
					numTransientResources += builder.GetTransientResourceSpan().size();

				currFrameTransientResourceArr.reserve(numTransientResources);
			}

			for (auto& builder : transientResourceBuilderArr)
			{
				for (auto&& resourcePtr : builder.GetTransientResourceSpan())
					currFrameTransientResourceArr.push_back(std::move(resourcePtr));
			}

			GetCurrentTransientResourceContainer() = std::move(currFrameTransientResourceArr);
		}
	}

	BlackboardElementTuple_T& FrameGraphBlackboard::GetCurrentBlackboardElementTuple()
	{
		const std::size_t currFrameIndex = (Util::Engine::GetCurrentFrameNumber() % mElementTupleArr.size());
		return mElementTupleArr[currFrameIndex];
	}

	const BlackboardElementTuple_T& FrameGraphBlackboard::GetCurrentBlackboardElementTuple() const
	{
		const std::size_t currFrameIndex = (Util::Engine::GetCurrentFrameNumber() % mElementTupleArr.size());
		return mElementTupleArr[currFrameIndex];
	}

	FrameGraphBlackboard::TransientResourceContainer& FrameGraphBlackboard::GetCurrentTransientResourceContainer()
	{
		const std::size_t currFrameIndex = (Util::Engine::GetCurrentFrameNumber() % mTransientResourceContainerArr.size());
		return mTransientResourceContainerArr[currFrameIndex];
	}

	const FrameGraphBlackboard::TransientResourceContainer& FrameGraphBlackboard::GetCurrentTransientResourceContainer() const
	{
		const std::size_t currFrameIndex = (Util::Engine::GetCurrentFrameNumber() % mTransientResourceContainerArr.size());
		return mTransientResourceContainerArr[currFrameIndex];
	}
}