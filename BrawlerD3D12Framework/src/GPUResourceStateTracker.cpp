module;
#include <optional>
#include <cassert>
#include <span>
#include <ranges>
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
			mBarrierMergerArr(),
			mResourceAlwaysDecays(DoesResourceAlwaysDecay(trackedResource))
		{
			const std::size_t subResourceCount = trackedResource.GetSubResourceCount();

			mBarrierMergerArr.reserve(subResourceCount);

			for (auto i : std::views::iota(0u, subResourceCount))
				mBarrierMergerArr.emplace_back(trackedResource, i);
		}

		void GPUResourceStateTracker::TrackGPUExecutionModule(const GPUExecutionModule& executionModule)
		{
			for (auto& barrierMerger : mBarrierMergerArr)
				TrackGPUExecutionModuleForSubResource(executionModule, barrierMerger);
			
			CheckForResourceStateDecay(executionModule);
		}

		GPUResourceEventCollection GPUResourceStateTracker::FinalizeStateTracking()
		{
			GPUResourceEventCollection eventCollection{};

			for (auto& barrierMerger : mBarrierMergerArr)
				eventCollection.MergeGPUResourceEventCollection(barrierMerger.FinalizeStateTracking());
			
			return eventCollection;
		}

		void GPUResourceStateTracker::TrackGPUExecutionModuleForSubResource(const GPUExecutionModule& executionModule, GPUSubResourceStateBarrierMerger& barrierMerger)
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
			const Brawler::CompositeEnum<GPUCommandQueueType> queuesUsingSubResource{ executionModule.GetQueuesUsingResource(barrierMerger.GetGPUResource(), barrierMerger.GetSubResourceIndex()) };
			const std::uint32_t numQueuesUsingSubResource = queuesUsingSubResource.CountOneBits();

			if (numQueuesUsingSubResource == 1) [[unlikely]]
			{
				const auto trackRenderPassesLambda = [&barrierMerger]<GPUCommandQueueType QueueType>(const GPUExecutionModule& executionModule)
				{
					const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassSpan{ executionModule.GetRenderPassSpan<QueueType>() };

					if (executionModule.IsResourceUsedInQueue<QueueType>(barrierMerger.GetGPUResource(), barrierMerger.GetSubResourceIndex()))
					{
						for (const auto& renderPassPtr : renderPassSpan)
							barrierMerger.TrackRenderPass(*renderPassPtr);
					}
				};

				trackRenderPassesLambda.operator()<GPUCommandQueueType::DIRECT>(executionModule);
				trackRenderPassesLambda.operator()<GPUCommandQueueType::COMPUTE>(executionModule);
				trackRenderPassesLambda.operator()<GPUCommandQueueType::COPY>(executionModule);
			}
			else if (barrierMerger.CanUseAdditionalBeginBarrierRenderPasses() && numQueuesUsingSubResource == 0) [[unlikely]]
			{
				// Of course, even if a GPUExecutionModule is not using a resource, we may still
				// be able to use its render passes as the beginning of split barriers.

				const auto addBeginBarrierPassLambda = [&barrierMerger]<GPUCommandQueueType QueueType>(const GPUExecutionModule& executionModule)
				{
					const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassSpan{ executionModule.GetRenderPassSpan<QueueType>() };

					if (!renderPassSpan.empty())
						barrierMerger.AddPotentialBeginBarrierRenderPass(*(renderPassSpan[0]));
				};

				addBeginBarrierPassLambda.operator()<GPUCommandQueueType::DIRECT>(executionModule);
				addBeginBarrierPassLambda.operator()<GPUCommandQueueType::COMPUTE>(executionModule);
				addBeginBarrierPassLambda.operator()<GPUCommandQueueType::COPY>(executionModule);
			}
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

			const bool resourceUsedInCopyQueue = executionModule.IsResourceUsedInQueue<GPUCommandQueueType::COPY>(mBarrierMergerArr[0].GetGPUResource());

			if constexpr (Util::General::IsDebugModeEnabled())
			{
				if (resourceUsedInCopyQueue)
				{
					for (auto& barrierMerger : mBarrierMergerArr)
					{
						const Brawler::CompositeEnum<GPUCommandQueueType> queuesUsingResource{ executionModule.GetQueuesUsingResource(barrierMerger.GetGPUResource(), barrierMerger.GetSubResourceIndex()) };
						assert(queuesUsingResource.CountOneBits() == 1 && "ERROR: It is an error to use an I_GPUResource simultaneously in both a COPY queue and either a DIRECT or a COMPUTE queue, even if the uses are for different sub-resources! This will result in *undefined behavior*!");
					}
				}
			}

			for (auto& barrierMerger : mBarrierMergerArr)
			{
				if (mResourceAlwaysDecays || resourceUsedInCopyQueue || barrierMerger.DoImplicitReadStateTransitionsAllowStateDecay())
					barrierMerger.DecayResourceState();
			}
		}
	}
}