module;

export module Brawler.D3D12.BaseResourceStateZoneOptimizerState;
import Brawler.D3D12.ResourceStateZoneOptimizerStateID;

export namespace Brawler
{
	namespace D3D12
	{
		class ResourceStateZoneOptimizer;
		struct ResourceStateZone;
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BaseResourceStateZoneOptimizerState
		{
		protected:
			explicit BaseResourceStateZoneOptimizerState(ResourceStateZoneOptimizer& optimizer);

		public:
			virtual ~BaseResourceStateZoneOptimizerState() = default;

		protected:
			void RequestResourceStateZoneDeletion(ResourceStateZone& stateZone);

			const I_GPUResource& GetGPUResource() const;

			template <ResourceStateZoneOptimizerStateID StateID>
				requires (StateID == ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE)
			void RequestOptimizerStateChange(ResourceStateZone& stateZone);

			template <ResourceStateZoneOptimizerStateID StateID>
				requires (StateID == ResourceStateZoneOptimizerStateID::IGNORE_RESOURCE_STATE_ZONE)
			void RequestOptimizerStateChange();

		private:
			void SendOptimizerStateChangeRequest(const ResourceStateZoneOptimizerStateID stateID, ResourceStateZone* stateZonePtr);

		private:
			ResourceStateZoneOptimizer* mOptimizerPtr;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <ResourceStateZoneOptimizerStateID StateID>
			requires (StateID == ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE)
		void BaseResourceStateZoneOptimizerState::RequestOptimizerStateChange(ResourceStateZone& stateZone)
		{
			SendOptimizerStateChangeRequest(StateID, &stateZone);
		}

		template <ResourceStateZoneOptimizerStateID StateID>
			requires (StateID == ResourceStateZoneOptimizerStateID::IGNORE_RESOURCE_STATE_ZONE)
		void BaseResourceStateZoneOptimizerState::RequestOptimizerStateChange()
		{
			SendOptimizerStateChangeRequest(StateID, nullptr);
		}
	}
}