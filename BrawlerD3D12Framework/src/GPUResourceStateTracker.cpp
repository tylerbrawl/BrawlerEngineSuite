module;
#include <optional>
#include <cassert>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceStateTracker;
import Brawler.CompositeEnum;
import Util.General;

namespace
{
	bool DoesResourceAlwaysDecay(const Brawler::D3D12::I_GPUResource& resource)
	{
		const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ resource.GetResourceDescription() };
		return (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER || (resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);
	}
}

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceStateTracker::GPUResourceStateTracker(I_GPUResource& trackedResource) :
			mBarrierMerger(trackedResource),
			mResourceAlwaysDecays(DoesResourceAlwaysDecay(trackedResource))
		{}

		void GPUResourceStateTracker::TrackGPUExecutionModule(const GPUExecutionModule& executionModule)
		{
			// Each time multiple queues are being used simultaneously, a sync point containing
			// a combination of all of the states of resources used across the queues is added.
			// This means that we do not need to actually track the state of a resource across
			// a GPUExecutionModule if said resource is being used in that module across more than
			// one queue.
			// 
			// In addition, most resources won't be used in a lot of render passes. By filtering 
			// out GPUExecutionModules which do not use the given resource, we can drastically
			// improve performance.
			const Brawler::CompositeEnum<GPUCommandQueueType> queuesUsingResource{ executionModule.GetQueuesUsingResource(mBarrierMerger.GetGPUResource()) };
			const std::uint32_t numQueuesUsingResource = queuesUsingResource.CountOneBits();

			if (numQueuesUsingResource == 1) [[unlikely]]
			{
				const auto trackRenderPassesLambda = [this]<GPUCommandQueueType QueueType>(const GPUExecutionModule& executionModule)
				{
					const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassSpan{ executionModule.GetRenderPassSpan<QueueType>() };

					if (executionModule.IsResourceUsedInQueue<QueueType>(mBarrierMerger.GetGPUResource()))
					{
						if constexpr (QueueType == GPUCommandQueueType::COPY)
						{
							if (mBarrierMerger.GetGPUResource().GetResourceDescription().Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
								int breakHere = -1;
						}
						
						for (const auto& renderPassPtr : renderPassSpan)
							mBarrierMerger.TrackRenderPass(*renderPassPtr);
					}
				};

				trackRenderPassesLambda.operator() < GPUCommandQueueType::DIRECT > (executionModule);
				trackRenderPassesLambda.operator() < GPUCommandQueueType::COMPUTE > (executionModule);
				trackRenderPassesLambda.operator() < GPUCommandQueueType::COPY > (executionModule);
			}
			else if (numQueuesUsingResource == 0) [[likely]]
			{
				// Of course, even if a GPUExecutionModule is not using a resource, we may still
				// be able to use its render passes as the beginning of split barriers.

				const auto addBeginBarrierPassLambda = [this]<GPUCommandQueueType QueueType>(const GPUExecutionModule & executionModule)
				{
					const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassSpan{ executionModule.GetRenderPassSpan<QueueType>() };

					if (!renderPassSpan.empty())
						mBarrierMerger.AddPotentialBeginBarrierRenderPass(*(renderPassSpan[0]));
				};

				addBeginBarrierPassLambda.operator() < GPUCommandQueueType::DIRECT > (executionModule);
				addBeginBarrierPassLambda.operator() < GPUCommandQueueType::COMPUTE > (executionModule);
				addBeginBarrierPassLambda.operator() < GPUCommandQueueType::COPY > (executionModule);
			}
			
			CheckForResourceStateDecay(executionModule);
		}

		GPUResourceEventCollection GPUResourceStateTracker::FinalizeStateTracking()
		{
			return mBarrierMerger.FinalizeStateTracking();
		}

		void GPUResourceStateTracker::CheckForResourceStateDecay(const GPUExecutionModule& executionModule)
		{
			// According to the MSDN, a resource's state will decay upon a call to 
			// ID3D12CommandQueue::ExecuteCommandLists() if any of the following conditions are
			// met:
			//
			// - The resource is either a buffer or a simultaneous-access texture.
			// - The resource is being used in a COPY queue. (I believe that it is an error to
			//   use a resource simultaneously in both a COPY queue and a DIRECT or COMPUTE
			//   queue in the D3D12 API. Regardless, we disallow that in the Brawler Engine.)
			// - The resource was implicitly promoted to a read-only state. (The MSDN does not
			//   state if the decay still occurs if this implicit promotion happened after an
			//   explicit transition to the COMMON state, but we assume that it does.)

			const bool resourceUsedInCopyQueue = executionModule.IsResourceUsedInQueue<GPUCommandQueueType::COPY>(mBarrierMerger.GetGPUResource());

			if constexpr (Util::General::IsDebugModeEnabled())
			{
				if (resourceUsedInCopyQueue)
				{
					const Brawler::CompositeEnum<GPUCommandQueueType> queuesUsingResource{ executionModule.GetQueuesUsingResource(mBarrierMerger.GetGPUResource()) };
					assert(queuesUsingResource.CountOneBits() == 1 && "ERROR: It is an error to use an I_GPUResource simultaneously in both a COPY queue and either a DIRECT or a COMPUTE queue! This will result in *undefined behavior* in Release builds!");
				}
			}

			if (mResourceAlwaysDecays || resourceUsedInCopyQueue || mBarrierMerger.DoImplicitReadStateTransitionsAllowStateDecay())
				mBarrierMerger.DecayResourceState();
		}
	}
}