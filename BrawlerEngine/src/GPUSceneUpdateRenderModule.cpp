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
import Brawler.VirtualTextureManagementPassCollection;

namespace Brawler
{
	void GPUSceneUpdateRenderModule::CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUploadContext>&& preparedBufferPtr)
	{
		mVTManagementSubModule.CommitGlobalTextureChanges(std::move(preparedBufferPtr));
	}

	bool GPUSceneUpdateRenderModule::IsRenderModuleEnabled() const
	{
		// Don't bother trying to create RenderPass instances for GPU scene updates if there are
		// no updates to perform this frame.
		
		return (mBufferUpdateSubModule.HasScheduledBufferUpdates() || mVTManagementSubModule.HasCommittedGlobalTextureChanges());
	}

	void GPUSceneUpdateRenderModule::BuildFrameGraph(D3D12::FrameGraphBuilder& builder)
	{
		const bool shouldPerformBufferUpdates = mBufferUpdateSubModule.HasScheduledBufferUpdates();
		const bool shouldPerformVirtualTextureUpdates = mVTManagementSubModule.HasCommittedGlobalTextureChanges();

		// The FrameGraph system should not call this function if GPUSceneUpdateRenderModule::IsRenderModuleEnabled()
		// returns false.
		assert(shouldPerformBufferUpdates || shouldPerformVirtualTextureUpdates);

		if (shouldPerformVirtualTextureUpdates)
			mVTManagementSubModule.PrepareForGlobalTextureUpdates();

		// We don't need to use std::optional here; we can just verify whether or not the RenderPass
		// instances contain valid values based on the boolean variables above.
		GPUSceneBufferUpdateSubModule::GPUSceneBufferUpdatePassTuple gpuSceneBufferUpdatesTuple{};
		VirtualTextureManagementSubModule::IndirectionTextureUpdatePass_T indirectionTextureUpdatePass{};
		VirtualTextureManagementPassCollection globalTextureUpdatePassCollection{};

		D3D12::FrameGraph& currFrameGraph{ builder.GetFrameGraph() };
		std::array<D3D12::FrameGraphBuilder, 2> extraBuildersArr{
			D3D12::FrameGraphBuilder{ currFrameGraph },
			D3D12::FrameGraphBuilder{ currFrameGraph }
		};

		// Create the RenderPass instances for updating global textures, indirection textures, and GPU scene
		// buffers concurrently.
		Brawler::JobGroup gpuSceneUpdatePassCreationGroup{};

		if (shouldPerformBufferUpdates) [[likely]]
		{
			D3D12::FrameGraphBuilder& assignedBuilder{ extraBuildersArr[0] };
			gpuSceneUpdatePassCreationGroup.AddJob([this, &gpuSceneBufferUpdatesTuple, &assignedBuilder] ()
			{
				gpuSceneBufferUpdatesTuple = mBufferUpdateSubModule.CreateGPUSceneBufferUpdateRenderPassTuple(assignedBuilder);
			});
		}

		if (shouldPerformVirtualTextureUpdates)
		{
			D3D12::FrameGraphBuilder& globalTextureUpdatePassBuilder{ extraBuildersArr[1] };
			gpuSceneUpdatePassCreationGroup.AddJob([this, &globalTextureUpdatePassCollection, &globalTextureUpdatePassBuilder] ()
			{
				globalTextureUpdatePassCollection = mVTManagementSubModule.CreateGlobalTextureChangeRenderPasses(globalTextureUpdatePassBuilder);
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

		if (shouldPerformVirtualTextureUpdates)
		{
			globalTextureUpdatePassCollection.MoveRenderPassesIntoBundle(gpuSceneUpdatePassBundle);
			builder.MergeFrameGraphBuilder(std::move(extraBuildersArr[1]));
		}

		builder.AddRenderPassBundle(std::move(gpuSceneUpdatePassBundle));
	}
}