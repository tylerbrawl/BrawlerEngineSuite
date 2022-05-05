module;
#include "DxDef.h"

export module Brawler.D3D12.I_ResourceStateZoneOptimizerState;
export import Brawler.D3D12.BaseResourceStateZoneOptimizerState;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		class I_ResourceStateZoneOptimizerState : public BaseResourceStateZoneOptimizerState
		{
		protected:
			explicit I_ResourceStateZoneOptimizerState(ResourceStateZoneOptimizer& optimizer);

		public:
			virtual ~I_ResourceStateZoneOptimizerState() = default;

			void ProcessResourceStateZone(ResourceStateZone& stateZone);
			void OnStateDecayBarrier();
		};
	}
}

// -----------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		I_ResourceStateZoneOptimizerState<DerivedType>::I_ResourceStateZoneOptimizerState(ResourceStateZoneOptimizer& optimizer) :
			BaseResourceStateZoneOptimizerState(optimizer)
		{}

		template <typename DerivedType>
		void I_ResourceStateZoneOptimizerState<DerivedType>::ProcessResourceStateZone(ResourceStateZone& stateZone)
		{
			static_cast<DerivedType*>(this)->ProcessResourceStateZone(stateZone);
		}

		template <typename DerivedType>
		void I_ResourceStateZoneOptimizerState<DerivedType>::OnStateDecayBarrier()
		{
			static_cast<DerivedType*>(this)->OnStateDecayBarrier();
		}
	}
}