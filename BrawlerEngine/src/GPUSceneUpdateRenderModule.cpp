module;
#include <memory>
#include <array>
#include <vector>
#include <optional>
#include <tuple>
#include <cassert>

module Brawler.GPUSceneUpdateRenderModule;
import Brawler.JobSystem;
import Brawler.D3D12.FrameGraphBuilding;

namespace Brawler
{
	void GPUSceneUpdateRenderModule::ScheduleGenericBufferUpdateForNextFrame(GenericPreFrameBufferUpdate&& preFrameUpdate)
	{
		mPreFrameUpdateSubModule.AddPreFrameBufferUpdate(std::move(preFrameUpdate));
	}
	
	bool GPUSceneUpdateRenderModule::IsRenderModuleEnabled() const
	{
		// Don't bother trying to create RenderPass instances for GPU scene updates if there are
		// no updates to perform this frame.
		
		return (mBufferUpdateSubModule.HasScheduledBufferUpdates() || mPreFrameUpdateSubModule.HasPendingUpdates());
	}

	void GPUSceneUpdateRenderModule::BuildFrameGraph(D3D12::FrameGraphBuilder& builder)
	{
		const bool shouldPerformBufferUpdates = mBufferUpdateSubModule.HasScheduledBufferUpdates();
		const bool shouldPerformGenericPreFrameUpdates = mPreFrameUpdateSubModule.HasPendingUpdates();

		// The FrameGraph system should not call this function if GPUSceneUpdateRenderModule::IsRenderModuleEnabled()
		// returns false.
		assert(shouldPerformBufferUpdates || shouldPerformGenericPreFrameUpdates);

		// We don't need to use std::optional here; we can just verify whether or not the RenderPass
		// instances contain valid values based on the boolean variables above.
		GPUSceneBufferUpdateSubModule::GPUSceneBufferUpdatePassTuple gpuSceneBufferUpdatesTuple{};
		GenericPreFrameUpdateSubModule::BufferUpdateRenderPass_T genericPreFrameBufferUpdatePass{};

		D3D12::FrameGraph& currFrameGraph{ builder.GetFrameGraph() };
		std::array<D3D12::FrameGraphBuilder, 2> extraBuildersArr{
			D3D12::FrameGraphBuilder{ currFrameGraph },
			D3D12::FrameGraphBuilder{ currFrameGraph }
		};

		// Create the RenderPass instances for executing generic pre-frame updates and GPU scene
		// buffer updates concurrently.
		Brawler::JobGroup gpuSceneUpdatePassCreationGroup{};

		if (shouldPerformBufferUpdates) [[likely]]
		{
			D3D12::FrameGraphBuilder& assignedBuilder{ extraBuildersArr[0] };
			gpuSceneUpdatePassCreationGroup.AddJob([this, &gpuSceneBufferUpdatesTuple, &assignedBuilder] ()
			{
				gpuSceneBufferUpdatesTuple = mBufferUpdateSubModule.CreateGPUSceneBufferUpdateRenderPassTuple(assignedBuilder);
			});
		}

		if (shouldPerformGenericPreFrameUpdates)
		{
			D3D12::FrameGraphBuilder& genericPreFrameBufferUpdatePassBuilder{ extraBuildersArr[1] };
			gpuSceneUpdatePassCreationGroup.AddJob([this, &genericPreFrameBufferUpdatePass, &genericPreFrameBufferUpdatePassBuilder] ()
			{
				genericPreFrameBufferUpdatePass = mPreFrameUpdateSubModule.CreateBufferUpdateRenderPass(genericPreFrameBufferUpdatePassBuilder);
			});
		}

		gpuSceneUpdatePassCreationGroup.ExecuteJobs();

		D3D12::RenderPassBundle gpuSceneUpdatePassBundle{};

		if (shouldPerformBufferUpdates) [[likely]]
		{
			const auto addBufferUpdatePassesLambda = [&gpuSceneUpdatePassBundle, &gpuSceneBufferUpdatesTuple]<GPUSceneBufferID BufferID>(this const auto& self)
			{
				if constexpr (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
				{
					auto& optionalRenderPass{ std::get<std::to_underlying(BufferID)>(gpuSceneBufferUpdatesTuple) };

					if (optionalRenderPass.has_value())
						gpuSceneUpdatePassBundle.AddRenderPass(std::move(*optionalRenderPass));

					constexpr GPUSceneBufferID NEXT_ID = static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) + 1);
					self.operator()<NEXT_ID>();
				}
			};
			addBufferUpdatePassesLambda.operator()<static_cast<GPUSceneBufferID>(0)>();

			builder.MergeFrameGraphBuilder(std::move(extraBuildersArr[0]));
		}

		if (shouldPerformGenericPreFrameUpdates)
		{
			gpuSceneUpdatePassBundle.AddRenderPass(std::move(genericPreFrameBufferUpdatePass));
			builder.MergeFrameGraphBuilder(std::move(extraBuildersArr[1]));
		}

		builder.AddRenderPassBundle(std::move(gpuSceneUpdatePassBundle));
	}
}