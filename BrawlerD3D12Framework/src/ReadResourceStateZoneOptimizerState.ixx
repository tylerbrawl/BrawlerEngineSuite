module;
#include <vector>

export module Brawler.D3D12.ReadResourceStateZoneOptimizerState;
import Brawler.D3D12.I_ResourceStateZoneOptimizerState;

export namespace Brawler
{
	namespace D3D12
	{
		class ReadResourceStateZoneOptimizerState final : public I_ResourceStateZoneOptimizerState<ReadResourceStateZoneOptimizerState>
		{
		public:
			ReadResourceStateZoneOptimizerState(ResourceStateZone& stateZoneToOptimize, ResourceStateZoneOptimizer& optimizer);

			void ProcessResourceStateZone(ResourceStateZone& stateZone);
			void OnStateDecayBarrier();

		private:
			void DeleteNullResourceStateZones();
			void MergeReadResourceStateZone(ResourceStateZone& stateZone);

		private:
			ResourceStateZone* mStateZoneToOptimize;
			std::vector<ResourceStateZone*> mPotentiallyDeleteableStateZoneArr;
		};
	}
}