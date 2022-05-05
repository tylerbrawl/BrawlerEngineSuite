module;
#include <atomic>
#include <cstdint>
#include <limits>
#include <algorithm>

module Brawler.D3D12.GPUResourceUsageTracker;
import Util.Engine;
import Util.Math;

namespace
{
	static constexpr std::uint32_t FRAMES_IN_FLIGHT_MASK = ((1 << Util::Engine::MAX_FRAMES_IN_FLIGHT) - 1);
}

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceUsageTracker::GPUResourceUsageTracker() :
			mResourceCreationFrameNum(Util::Engine::GetCurrentFrameNumber()),
			mLastUseFrameNum(mResourceCreationFrameNum - 1),
			mResourceUsageBitMask(0)
		{}

		void GPUResourceUsageTracker::MarkAsUsedForCurrentFrame()
		{
			// Check if this is the first time which the resource's use has been counted for this frame.
			const std::uint64_t currFrameNumber = Util::Engine::GetCurrentFrameNumber();
			const std::uint64_t previousLastUseValue = mLastUseFrameNum.exchange(currFrameNumber, std::memory_order::relaxed);

			if (previousLastUseValue == currFrameNumber)
				return;

			// We acquired the first update for this frame.
			const std::uint64_t shiftAmount = (currFrameNumber - previousLastUseValue);

			std::uint32_t usageBitMask = mResourceUsageBitMask.load(std::memory_order::relaxed);
			usageBitMask <<= shiftAmount;

			// The resource is used this frame, so we update the bitmask for it.
			usageBitMask |= 0x1;

			mResourceUsageBitMask.store(usageBitMask, std::memory_order::relaxed);
		}

		float GPUResourceUsageTracker::GetResourceUsageMetric() const
		{
			// Tracking GPU resource usage is done via bit manipulation.

			const std::uint64_t framesSinceCreation = (Util::Engine::GetCurrentFrameNumber() - mResourceCreationFrameNum);
			const std::uint32_t resourceUsageBitMask = mResourceUsageBitMask.load(std::memory_order::relaxed);

			// We can then calculate the resource usage metric. We define this to be the number of times
			// in which the resource was used in the last 32 frames divided by the minimum of the number
			// of frames in which the resource has been alive and 32.
			return (static_cast<float>(Util::Math::CountOneBits(resourceUsageBitMask)) / static_cast<float>(std::min<std::uint64_t>(framesSinceCreation + 1, 32)));
		}

		bool GPUResourceUsageTracker::IsResourceSafeToEvict() const
		{
			const std::uint32_t usageBitMask = mResourceUsageBitMask.load(std::memory_order::relaxed);

			return ((usageBitMask & FRAMES_IN_FLIGHT_MASK) == 0);
		}
	}
}