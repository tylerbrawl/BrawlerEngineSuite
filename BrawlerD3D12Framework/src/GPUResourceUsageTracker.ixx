module;
#include <atomic>
#include <cstdint>

export module Brawler.D3D12.GPUResourceUsageTracker;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceUsageTracker
		{
		public:
			GPUResourceUsageTracker();

			void MarkAsUsedForCurrentFrame();

			/// <summary>
			/// Calculates a heuristic to determine the usage of the associated I_GPUResource
			/// instance. 
			/// 
			/// Specifically, let C be the resource's age in frames, and let U be the number of 
			/// times in which the resource was used in the past 32 frames. Then, this function 
			/// returns U / std::min(C, 32).
			/// </summary>
			/// <returns>
			/// The function returns a heuristic to determine the usage of the associated
			/// I_GPUResource instance. Read the summary for details as to how the heuristic is
			/// calculated.
			/// </returns>
			float GetResourceUsageMetric() const;

			bool IsResourceSafeToEvict() const;

		private:
			std::uint64_t mResourceCreationFrameNum;
			std::atomic<std::uint64_t> mLastUseFrameNum;
			std::atomic<std::uint32_t> mResourceUsageBitMask;
		};
	}
}