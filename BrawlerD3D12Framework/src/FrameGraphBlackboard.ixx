module;
#include <array>
#include <mutex>
#include <variant>

export module Brawler.D3D12.FrameGraphBlackboard;
import Brawler.D3D12.FrameGraphBlackboardComponent;
import Brawler.D3D12.FrameGraphBlackboardComponentMap;

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBlackboard
		{
		private:
			struct ComponentEntry
			{
				FrameGraphBlackboardComponentVariantType ComponentData;
				mutable std::mutex CriticalSection;
			};

		public:
			FrameGraphBlackboard();

			FrameGraphBlackboard(const FrameGraphBlackboard& rhs) = delete;
			FrameGraphBlackboard& operator=(const FrameGraphBlackboard& rhs) = delete;

			FrameGraphBlackboard(FrameGraphBlackboard&& rhs) noexcept = default;
			FrameGraphBlackboard& operator=(FrameGraphBlackboard&& rhs) noexcept = default;

			template <FrameGraphBlackboardComponent ComponentID>
			FrameGraphBlackboardComponentType<ComponentID>& GetComponent();

			template <FrameGraphBlackboardComponent ComponentID>
			const FrameGraphBlackboardComponentType<ComponentID>& GetComponent() const;

			template <FrameGraphBlackboardComponent ComponentID>
			void SetComponent(const FrameGraphBlackboardComponentType<ComponentID>& data);

			template <FrameGraphBlackboardComponent ComponentID>
			void SetComponent(FrameGraphBlackboardComponentType<ComponentID>&& data);

			void ClearBlackboard();

		private:
			template <FrameGraphBlackboardComponent CurrComponent>
			void ClearBlackboardIMPL();

		private:
			std::array<ComponentEntry, std::to_underlying(FrameGraphBlackboardComponent::COUNT_OR_ERROR)> mComponentArr;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <FrameGraphBlackboardComponent ComponentID>
		FrameGraphBlackboardComponentType<ComponentID>& FrameGraphBlackboard::GetComponent()
		{
			static constexpr std::size_t ARRAY_INDEX = std::to_underlying(ComponentID);

			std::scoped_lock<std::mutex> lock{ mComponentArr[ARRAY_INDEX].CriticalSection };
			return std::get<FrameGraphBlackboardComponentType<ComponentID>>(mComponentArr[ARRAY_INDEX].ComponentData);
		}

		template <FrameGraphBlackboardComponent ComponentID>
		const FrameGraphBlackboardComponentType<ComponentID>& FrameGraphBlackboard::GetComponent() const
		{
			static constexpr std::size_t ARRAY_INDEX = std::to_underlying(ComponentID);

			std::scoped_lock<std::mutex> lock{ mComponentArr[ARRAY_INDEX].CriticalSection };
			return std::get<FrameGraphBlackboardComponentType<ComponentID>>(mComponentArr[ARRAY_INDEX].ComponentData);
		}

		template <FrameGraphBlackboardComponent ComponentID>
		void FrameGraphBlackboard::SetComponent(const FrameGraphBlackboardComponentType<ComponentID>& data)
		{
			static constexpr std::size_t ARRAY_INDEX = std::to_underlying(ComponentID);

			std::scoped_lock<std::mutex> lock{ mComponentArr[ARRAY_INDEX].CriticalSection };
			mComponentArr[ARRAY_INDEX].ComponentData = data;
		}

		template <FrameGraphBlackboardComponent ComponentID>
		void FrameGraphBlackboard::SetComponent(FrameGraphBlackboardComponentType<ComponentID>&& data)
		{
			static constexpr std::size_t ARRAY_INDEX = std::to_underlying(ComponentID);

			std::scoped_lock<std::mutex> lock{ mComponentArr[ARRAY_INDEX].CriticalSection };
			mComponentArr[ARRAY_INDEX].ComponentData = std::move(data);
		}
	}
}