module;
#include <vector>
#include <span>
#include <optional>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <array>
#include <ranges>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceStateManagement:GPUResourceUsageAnalyzer;
import Brawler.D3D12.ResourceStateZoneMap;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.ResourceStateZone;
import Brawler.D3D12.FrameGraphCompilation;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.CompositeEnum;
import Util.D3D12;

namespace
{
	struct ResourceUsageInfo
	{
		std::vector<Brawler::D3D12::ResourceStateZone> ResourceStateZones;
		bool ContainsNonNullZone;
	};

	template <Brawler::D3D12::GPUCommandQueueType QueueType>
	ResourceUsageInfo GetResourceUsageInfoForRenderPasses(const Brawler::D3D12::I_GPUResource& resource, const Brawler::D3D12::GPUExecutionModule& executionModule)
	{
		ResourceUsageInfo usageInfo{
			.ResourceStateZones{},
			.ContainsNonNullZone = false
		};

		for (const auto& renderPass : executionModule.GetRenderPassSpan<QueueType>())
		{
			bool resourceUsedInPass = false;
			
			for (const auto& dependency : renderPass->GetResourceDependencies())
			{
				if (dependency.ResourcePtr == &resource)
				{
					usageInfo.ResourceStateZones.push_back(Brawler::D3D12::ResourceStateZone{
						.RequiredState{dependency.RequiredState},
						.EntranceRenderPass{renderPass.get()},
						.QueueType = QueueType,
						.ExecutionModule = &executionModule,
						.IsImplicitTransition = false
					});

					resourceUsedInPass = true;
					usageInfo.ContainsNonNullZone = true;

					break;
				}
			}

			if (!resourceUsedInPass)
				usageInfo.ResourceStateZones.push_back(Brawler::D3D12::ResourceStateZone{
					.RequiredState{},
					.EntranceRenderPass{renderPass.get()},
					.QueueType = QueueType,
					.ExecutionModule = &executionModule,
					.IsImplicitTransition = false
				});
		}

		return usageInfo;
	}

	bool DoesResourceAlwaysDecay(const Brawler::D3D12::I_GPUResource& resource)
	{
		const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ resource.GetResourceDescription() };
		return (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER || (resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);
	}

	std::optional<D3D12_RESOURCE_STATES> GetFinalResourceState(const std::span<const ResourceUsageInfo* const, 3> usageInfoArr)
	{
		// We don't allow a resource to be used in both the copy queue and the direct and/or 
		// compute queue(s) simultaneously. So, if we want to check if the resource is used in 
		// multiple queues, we just need to check the direct and compute case.
		const ResourceUsageInfo& directQueueUsageInfo{ *(usageInfoArr[0]) };
		const ResourceUsageInfo& computeQueueUsageInfo{ *(usageInfoArr[1]) };
		const ResourceUsageInfo& copyQueueUsageInfo{ *(usageInfoArr[2]) };

		const bool resourceUsedInMultipleQueues = (directQueueUsageInfo.ContainsNonNullZone && computeQueueUsageInfo.ContainsNonNullZone);
		
		if (!resourceUsedInMultipleQueues)
		{
			const auto getFinalStateInSingleQueueLambda = [] (const ResourceUsageInfo* const usageInfo) -> std::optional<D3D12_RESOURCE_STATES>
			{
				if (!usageInfo->ContainsNonNullZone)
					return std::optional<D3D12_RESOURCE_STATES>{};

				D3D12_RESOURCE_STATES trackedState{};

				for (const auto& stateZone : usageInfo->ResourceStateZones)
				{
					if (!stateZone.IsNull())
					{
						const D3D12_RESOURCE_STATES stateZoneRequiredState = *(stateZone.RequiredState);

						if (stateZoneRequiredState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && Util::D3D12::IsValidReadState(trackedState | stateZoneRequiredState))
							trackedState |= stateZoneRequiredState;
						else
							trackedState = stateZoneRequiredState;
					}
				}

				return std::optional<D3D12_RESOURCE_STATES>{ trackedState };
			};

			for (const auto usageInfo : usageInfoArr)
			{
				const std::optional<D3D12_RESOURCE_STATES> trackedState{ getFinalStateInSingleQueueLambda(usageInfo) };

				if (trackedState.has_value())
					return trackedState;
			}

			return std::optional<D3D12_RESOURCE_STATES>{};
		}
		else
		{
			D3D12_RESOURCE_STATES combinedReadStates{};

			for (const auto usageInfo : usageInfoArr | std::views::filter([](const ResourceUsageInfo* const usageInfo) { return (usageInfo->ContainsNonNullZone); }))
			{
				for (const auto& stateZone : usageInfo->ResourceStateZones | std::views::filter([] (const Brawler::D3D12::ResourceStateZone& stateZone) { return !stateZone.IsNull(); }))
					combinedReadStates |= *(stateZone.RequiredState);
			}

			assert(combinedReadStates != 0 && Util::D3D12::IsValidReadState(combinedReadStates));
			return std::optional<D3D12_RESOURCE_STATES>{ combinedReadStates };
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceUsageAnalyzer::GPUResourceUsageAnalyzer(I_GPUResource& resource) :
			mResourcePtr(&resource),
			mCurrResourceState(resource.GetCurrentResourceState()),
			mEventManager(),
			mResourceAlwaysDecays(DoesResourceAlwaysDecay(resource))
		{
#ifdef _DEBUG
			const D3D12_HEAP_TYPE resourceHeapType{ resource.GetHeapType() };
			assert(resourceHeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT && "ERROR: An attempt was made to track the state of a resource created in either a D3D12_HEAP_TYPE_UPLOAD or D3D12_HEAP_TYPE_READBACK heap! (This is unnecessary because resources created in these heaps can never transition out of their initial states. Tracking the resources anyways will likely lead to D3D12 device removal if actual transitions are performed.)");
#endif // _DEBUG
		}

		void GPUResourceUsageAnalyzer::TraverseFrameGraph(const std::span<const GPUExecutionModule> executionModuleSpan)
		{
			ResourceStateZoneMap stateZoneMap{ *mResourcePtr };

			for (const auto& executionModule : executionModuleSpan)
				TrackResourceUsageInExecutionModule(executionModule, stateZoneMap);

			stateZoneMap.OptimizeResourceStateZones();

			mEventManager = stateZoneMap.CreateGPUResourceEventManager();
		}

		GPUResourceEventManager GPUResourceUsageAnalyzer::ExtractGPUResourceEventManager()
		{
			return std::move(mEventManager);
		}

		void GPUResourceUsageAnalyzer::TrackResourceUsageInExecutionModule(const GPUExecutionModule& executionModule, ResourceStateZoneMap& stateZoneMap)
		{
			const ResourceUsageInfo directQueueUsageInfo{ GetResourceUsageInfoForRenderPasses<GPUCommandQueueType::DIRECT>(*mResourcePtr, executionModule) };
			const ResourceUsageInfo computeQueueUsageInfo{ GetResourceUsageInfoForRenderPasses<GPUCommandQueueType::COMPUTE>(*mResourcePtr, executionModule) };
			const ResourceUsageInfo copyQueueUsageInfo{ GetResourceUsageInfoForRenderPasses<GPUCommandQueueType::COPY>(*mResourcePtr, executionModule) };

			assert(!copyQueueUsageInfo.ContainsNonNullZone || (!directQueueUsageInfo.ContainsNonNullZone && !computeQueueUsageInfo.ContainsNonNullZone) && "ERROR: An attempt was made to use a resource simultaneously in both the copy queue and the direct and/or compute queue(s)!");

			// We don't allow a resource to be used in both the copy queue and the direct and/or 
			// compute queue(s) simultaneously. So, if we want to check if the resource is used in 
			// multiple queues, we just need to check the direct and compute case.
			const bool resourceUsedInMultipleQueues = (directQueueUsageInfo.ContainsNonNullZone && computeQueueUsageInfo.ContainsNonNullZone);

			const std::array<const ResourceUsageInfo*, 3> usageInfoArr{
				&directQueueUsageInfo,
				&computeQueueUsageInfo,
				&copyQueueUsageInfo
			};

			// The purpose of a ResourceStateZone is to let the GPUExecutionModule know during 
			// recording of GPU commands that a given resource needs a transition barrier
			// (excepting, of course, implicit state transitions). If a resource is used in both
			// the direct queue and the compute queue, then the FrameGraphBuilder will inject
			// a RenderPassBundle whose sole RenderPass transitions said resource to the combined
			// read state of all uses of the resource across these queues. This injected
			// RenderPass is known as a sync point.
			//
			// The sync point is handled just like every other RenderPass, so its resource
			// dependencies will be accounted for by the GPUResourceUsageAnalyzer just like every
			// other RenderPass. Since the sync point will transition the resource for its entire
			// use across both of these queues, we don't need to add the ResourceStateZones for
			// the RenderPass instances which it prepares resources for.
			if (!resourceUsedInMultipleQueues)
			{
				auto nonNullZoneResult = std::ranges::find_if(usageInfoArr, [] (const ResourceUsageInfo* const usageInfo) { return (usageInfo->ContainsNonNullZone); });

				if (nonNullZoneResult != usageInfoArr.end())
					stateZoneMap.AddResourceStateZones(std::span<const ResourceStateZone>{(*nonNullZoneResult)->ResourceStateZones});
				else
				{
					// If a resource is used in none of the queues, then we should choose the most
					// capable queue which is used in the GPUExecutionModule. This will allow us
					// to potentially add more split barriers.
					
					auto nonEmptyResult = std::ranges::find_if(usageInfoArr, [] (const ResourceUsageInfo* const usageInfo) { return (!usageInfo->ResourceStateZones.empty()); });
					assert(nonEmptyResult != usageInfoArr.end() && "ERROR: An empty GPUExecutionModule was created!");

					stateZoneMap.AddResourceStateZones(std::span<const ResourceStateZone>{(*nonEmptyResult)->ResourceStateZones});
				}
			}

			// Since every GPUExecutionModule is a call to ExecuteCommandLists(), we need to check
			// for implicit state decay here. There are number of instances in which a resource
			// will decay:

			//   - The resource is either a buffer or a simultaneous-access texture.
			//   - The resource is being used in the copy queue.
			if (mResourceAlwaysDecays || copyQueueUsageInfo.ContainsNonNullZone)
			{
				stateZoneMap.AddStateDecayBarrier();
				mCurrResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

				return;
			}

			//   - The resource was implicitly promoted to a read-only state.
			const D3D12_RESOURCE_STATES previousResourceState = mCurrResourceState;

			// We'll be taking a bit-wise combination of all of the states which the resource has been
			// in for this GPUExecutionModule. If the resource can implicitly transition to all of these
			// states, then it *may* be able to decay.
			//
			// The reason this works is because a resource which meets neither of the first two
			// conditions will still decay if the only transitions applied to it were implicit read-only
			// transitions. So, if we find a state which it could not have possibly performed an implicit
			// transition to, then we know that it must have used an explicit transition. Thus, the resource
			// will not decay in that case.
			D3D12_RESOURCE_STATES combinedStates{};

			for (const auto usageInfo : usageInfoArr | std::views::filter([] (const ResourceUsageInfo* const usageInfo) { return (usageInfo->ContainsNonNullZone); }))
			{
				for (const auto& stateZone : usageInfo->ResourceStateZones | std::views::filter([] (const ResourceStateZone& stateZone) { return !stateZone.IsNull(); }))
					combinedStates |= *(stateZone.RequiredState);
			}

			const bool isResourceDecayPossible = (previousResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && combinedStates != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON &&
				Util::D3D12::IsImplicitStateTransitionPossible(*mResourcePtr, combinedStates));

			if (isResourceDecayPossible)
			{
				stateZoneMap.AddStateDecayBarrier();
				mCurrResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

				return;
			}

			// If all of the conditions failed, then the resource will *NOT* decay back to the
			// D3D12_RESOURCE_STATE_COMMON state.
			const std::optional<D3D12_RESOURCE_STATES> finalResourceState{ GetFinalResourceState(std::span<const ResourceUsageInfo* const, 3>{ usageInfoArr }) };

			if (finalResourceState.has_value())
				mCurrResourceState = *finalResourceState;
		}
	}
}