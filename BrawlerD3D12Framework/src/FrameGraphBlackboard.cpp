module;
#include <array>
#include <variant>
#include <mutex>

module Brawler.D3D12.FrameGraphBlackboard;

namespace Brawler
{
	namespace D3D12
	{
		template <FrameGraphBlackboardComponent CurrComponent>
		void FrameGraphBlackboard::ClearBlackboardIMPL()
		{
			if constexpr (CurrComponent != FrameGraphBlackboardComponent::COUNT_OR_ERROR)
			{
				ComponentEntry& entry{ mComponentArr[std::to_underlying(CurrComponent)] };

				{
					std::scoped_lock<std::mutex> lock{ entry.CriticalSection };
					entry.ComponentData = FrameGraphBlackboardComponentType<CurrComponent>{};
				}

				ClearBlackboardIMPL<static_cast<FrameGraphBlackboardComponent>(std::to_underlying(CurrComponent) + 1)>();
			}
		}

		FrameGraphBlackboard::FrameGraphBlackboard() :
			mComponentArr()
		{
			ClearBlackboard();
		}

		void FrameGraphBlackboard::ClearBlackboard()
		{
			ClearBlackboardIMPL<static_cast<FrameGraphBlackboardComponent>(0)>();
		}
	}
}