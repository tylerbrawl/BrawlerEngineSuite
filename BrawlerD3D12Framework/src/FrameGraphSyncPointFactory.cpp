module;
#include <vector>
#include <unordered_map>
#include <span>
#include <cassert>
#include <optional>
#include <ranges>
#include <string_view>
#include "DxDef.h"

module Brawler.D3D12.FrameGraphSyncPointFactory;
import Util.D3D12;
import Brawler.D3D12.RenderPass;

namespace
{
	static constexpr std::string_view SYNC_POINT_RENDER_PASS_NAME{ "[Brawler Engine Internal Sync Point]" };
	static constexpr Util::D3D12::PIXEventColor_T SYNC_POINT_PIX_EVENT_COLOR = Util::D3D12::CalculatePIXColor(0xFF, 0x00, 0x00);
}

namespace Brawler
{
	namespace D3D12
	{
		CrossQueueGPUResourceTracker::CrossQueueGPUResourceTracker(I_GPUResource& resource) :
			mSubResourceInfoArr(),
			mResourcePtr(std::addressof(resource))
		{
			mSubResourceInfoArr.resize(resource.GetSubResourceCount());
		}

		void CrossQueueGPUResourceTracker::AssertResourceUsageValidity() const
		{
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				for (const auto& subResourceInfo : mSubResourceInfoArr)
				{
					if (subResourceInfo.UsedQueues.CountOneBits() > 1)
					{
						// If a sub-resource is being used in multiple queues simultaneously, then it must only be used in
						// a read-only state.
						assert(Util::D3D12::IsValidReadState(subResourceInfo.CombinedState) && "ERROR: Sub-resources used simultaneously in both the DIRECT and COMPUTE queues *MUST* be used only in a read-only state!");

						// An explicit transition of a sub-resource to the COMMON or PRESENT state is not allowed if the
						// sub-resource is being used in multiple queues. However, since (ANY_VALID_READ_STATE | COMMON)
						// == ANY_VALID_READ_STATE, whether or not a COMMON state transition was made cannot be determined
						// just be looking at subResourceInfo.CombinedState. Therefore, we have a separate variable dedicated
						// to checking for this: subResourceInfo.UsesExplicitCommonStateTransition.
						// 
						// The reason why we do not allow a transition to the COMMON state is because of implicit state
						// promotion. If a sub-resource is transitioned to the COMMON state before being used in multiple queues
						// and one of the queues then uses it as, say, an unordered-access resource in a shader, then the
						// D3D12 API will implicitly allow this without a transition.
						//
						// It is worth noting that this member of SharedSubResourceInfo is not included in Release builds,
						// since we assume that the user is following the rules.
						assert(!subResourceInfo.UsesExplicitCommonStateTransition && "ERROR: An explicit transition to the D3D12_RESOURCE_STATE_COMMON (or D3D12_RESOURCE_STATE_PRESENT) state is not allowed for sub-resources being used simultaneously in both the DIRECT and COMPUTE queues! (This will result in *undefined* behavior!)");
					}
				}
			}
		}

		void CrossQueueGPUResourceTracker::AddSyncPointResourceDependencies(std::vector<FrameGraphResourceDependency>& resourceDependencyArr) const
		{
			for (auto i : std::views::iota(0u, mSubResourceInfoArr.size()))
			{
				// We only need to add a sync point resource dependency if a sub-resource is either being used simultaneously
				// in a DIRECT queue and a COMPUTE queue or it is being used in a COPY queue. We can actually elide sync points
				// if all of the resources being used in a RenderPassBundle implicitly decay to the COMMON resource state.
				// For those resources, this function is never called.

				const CurrentSharedSubResourceInfo& subResourceInfo{ mSubResourceInfoArr[i] };
				
				if (subResourceInfo.UsedQueues.ContainsAllFlags(GPUCommandQueueType::DIRECT | GPUCommandQueueType::COMPUTE) || subResourceInfo.UsedQueues.ContainsAnyFlag(GPUCommandQueueType::COPY))
					resourceDependencyArr.push_back(FrameGraphResourceDependency{
						.ResourcePtr = mResourcePtr,
						.RequiredState = subResourceInfo.CombinedState,
						.SubResourceIndex = static_cast<std::uint32_t>(i)
					});
			}
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		std::optional<RenderPassBundle> FrameGraphSyncPointFactory::CreateSyncPoint() const
		{
			if (mCrossQueueTrackerMap.empty()) [[unlikely]]
				return std::optional<RenderPassBundle>{};
			
			// Check that the sub-resources are being used properly, but only in Debug
			// builds.
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				for (const auto& [resourcePtr, crossQueueResourceTracker] : mCrossQueueTrackerMap)
					crossQueueResourceTracker.AssertResourceUsageValidity();
			}
			
			std::vector<FrameGraphResourceDependency> syncPointDependencyArr{};

			for (const auto& [resourcePtr, crossQueueResourceTracker] : mCrossQueueTrackerMap)
			{
				// Don't bother adding sync point resource dependencies for resources which are
				// guaranteed to decay to the COMMON state after an ExecuteCommandLists() call.
				const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ resourcePtr->GetResourceDescription() };
				const bool resourceAlwaysDecays = (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER || (resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);

				if (!resourceAlwaysDecays)
					crossQueueResourceTracker.AddSyncPointResourceDependencies(syncPointDependencyArr);
			}

			// Do not create a sync point if we do not have any sync point resource dependencies.
			if (syncPointDependencyArr.empty()) [[likely]]
				return std::optional<RenderPassBundle>{};

			RenderPass<GPUCommandQueueType::DIRECT> syncPointRenderPass{};
			syncPointRenderPass.SetRenderPassName(SYNC_POINT_RENDER_PASS_NAME);
			syncPointRenderPass.SetPIXEventColor(SYNC_POINT_PIX_EVENT_COLOR);

			for (auto&& dependency : syncPointDependencyArr)
				syncPointRenderPass.AddResourceDependency(std::move(dependency));

			RenderPassBundle syncPointPassBundle{};
			syncPointPassBundle.AddRenderPass(std::move(syncPointRenderPass));

			return std::optional<RenderPassBundle>{ std::move(syncPointPassBundle) };
		}
	}
}