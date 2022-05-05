module;

export module Brawler.D3D12.IgnoreResourceStateZoneOptimizerState;
import Brawler.D3D12.I_ResourceStateZoneOptimizerState;

export namespace Brawler
{
	namespace D3D12
	{
		class IgnoreResourceStateZoneOptimizerState final : public I_ResourceStateZoneOptimizerState<IgnoreResourceStateZoneOptimizerState>
		{
		public:
			explicit IgnoreResourceStateZoneOptimizerState(ResourceStateZoneOptimizer& optimizer);

			void ProcessResourceStateZone(ResourceStateZone& stateZone);
			void OnStateDecayBarrier();
		};
	}
}