module;
#include <vector>

module Brawler.D3D12.GPUCommandContextGroup;

namespace Brawler
{
	namespace D3D12
	{
		Brawler::CompositeEnum<GPUCommandQueueType> GPUCommandContextGroup::GetUsedQueues() const
		{
			return mUsedQueuesTracker;
		}
	}
}