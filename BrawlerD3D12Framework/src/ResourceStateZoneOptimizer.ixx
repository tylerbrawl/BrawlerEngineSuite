module;
#include <vector>
#include <memory>
#include <span>
#include <optional>
#include <variant>

export module Brawler.D3D12.ResourceStateZoneOptimizer;
import Brawler.PolymorphicAdapter;
export import Brawler.ResourceStateZoneOptimizerStateTraits;
import Brawler.D3D12.I_ResourceStateZoneOptimizerState;
import Brawler.D3D12.ResourceStateZoneOptimizerStateID;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class ResourceStateZoneOptimizer
		{
		private:
			struct StateChangeParams
			{
				ResourceStateZoneOptimizerStateID StateID;
				ResourceStateZone* StateZone;
			};

		private:
			friend class BaseResourceStateZoneOptimizerState;

		public:
			explicit ResourceStateZoneOptimizer(const I_GPUResource& resource);

			ResourceStateZoneOptimizer(const ResourceStateZoneOptimizer& rhs) = delete;
			ResourceStateZoneOptimizer& operator=(const ResourceStateZoneOptimizer& rhs) = delete;

			ResourceStateZoneOptimizer(ResourceStateZoneOptimizer&& rhs) noexcept = default;
			ResourceStateZoneOptimizer& operator=(ResourceStateZoneOptimizer && rhs) noexcept = default;

			void ProcessResourceStateZone(ResourceStateZone& stateZone);
			void OnStateDecayBarrier();
			std::span<ResourceStateZone* const> GetResourceStateZonesToDelete() const;

		private:
			void AddResourceStateZoneForDeletion(ResourceStateZone& stateZone);
			void RequestOptimizerStateChange(StateChangeParams&& stateChangeParams);

			const I_GPUResource& GetGPUResource() const;

			void ChangeOptimizerState();

		private:
			Brawler::PolymorphicAdapter<I_ResourceStateZoneOptimizerState> mCurrState;
			std::vector<ResourceStateZone*> mStateZonesToDeleteArr;
			std::optional<StateChangeParams> mRequestedStateChange;
			const I_GPUResource* mResource;
		};
	}
}