module;
#include <tuple>
#include <atomic>
#include <optional>

// Riddle me this: As of writing this comment, this source file doesn't build unless we
// #include the <vector> header file. Can you spot the usage of std::vector?
#include <vector>

module Brawler.GPUSceneBufferUpdateSubModule;

namespace
{
	/*
		Updating GPU Scene Buffers
		---------------------------

		During frame graph building, calling code interacts with GPUSceneBufferUpdater instances in
		order to update a specific element within a GPU scene buffer. Internally, GPUSceneBufferUpdater
		instances will dispatch a GPUSceneBufferUpdateOperation containing both the data which is
		to be sent to the GPU and the D3D12::BufferCopyRegion representing the destination copy
		region.

		The GPUSceneBufferUpdateOperation is submitted to the GPUSceneUpdateRenderModule, which in
		turn passes it to its GPUSceneBufferUpdateSubModule instance, which finally assigns it to the
		relevant GPUSceneBufferUpdateMonitor. The changes will then be submitted to the GPU the next
		time frame graph building is initiated.

		During command list recording, the data is copied from the created upload buffer(s) into the specified
		GPU scene buffer regions on the GPU timeline. Since this copy is (and must) be done on the GPU
		timeline, we guarantee that we modify the data only after the previous frame's commands have stopped
		reading it.

		Data is copied into GPUSceneBufferUpdateOperation instances by value. This ensures thread safety
		and greatly simplifies lifetime management, as calling code is not required to keep GPUSceneBufferUpdater
		instances alive until it can be inferred that the data has finished being copied into an upload buffer.
	*/
}

namespace Brawler
{
	bool GPUSceneBufferUpdateSubModule::HasScheduledBufferUpdates() const
	{
		bool monitorWithUpdatesFound = false;

		const auto checkMonitorsForUpdatesLambda = [this, &monitorWithUpdatesFound]<GPUSceneBufferID BufferID>(this const auto& self)
		{
			if constexpr (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
			{
				if (std::get<GPUSceneBufferUpdateMonitor<BufferID>>(mMonitorTuple).HasScheduledBufferUpdates())
					monitorWithUpdatesFound = true;
				else
				{
					constexpr GPUSceneBufferID NEXT_ID = static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) + 1);
					self.operator()<NEXT_ID>();
				}
			}
		};
		checkMonitorsForUpdatesLambda.operator()<static_cast<GPUSceneBufferID>(0)>();

		return monitorWithUpdatesFound;
	}

	GPUSceneBufferUpdateSubModule::GPUSceneBufferUpdatePassTuple GPUSceneBufferUpdateSubModule::CreateGPUSceneBufferUpdateRenderPassTuple(D3D12::FrameGraphBuilder& builder)
	{
		UpdateControlBlock expectedControlBlock{ mControlBlock.load(std::memory_order::relaxed) };
		UpdateControlBlock desiredControlBlock{};

		do
		{
			desiredControlBlock = UpdateControlBlock{
				.NumThreadsUpdating = expectedControlBlock.NumThreadsUpdating,
				.BlockUpdates = true
			};
		} while (!mControlBlock.compare_exchange_weak(expectedControlBlock, desiredControlBlock, std::memory_order::relaxed));

		// Wait for the other threads to finish their updates. We don't want to start executing
		// other CPU jobs because we don't expect this wait to take that long.
		expectedControlBlock = desiredControlBlock;

		while (expectedControlBlock.NumThreadsUpdating > 0)
		{
			mControlBlock.wait(expectedControlBlock, std::memory_order::relaxed);
			expectedControlBlock = mControlBlock.load(std::memory_order::relaxed);
		}

		GPUSceneBufferUpdatePassTuple renderPassTuple{};
		const auto createRenderPassesLambda = [this, &renderPassTuple, &builder]<GPUSceneBufferID BufferID>(this const auto& self)
		{
			if constexpr (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
			{
				auto& currUpdateMonitor{ std::get<GPUSceneBufferUpdateMonitor<BufferID>>(mMonitorTuple) };

				if (currUpdateMonitor.HasScheduledBufferUpdates())
					std::get<std::optional<GPUSceneBufferUpdateMonitor<BufferID>::BufferUpdateRenderPass_T>>(renderPassTuple) = currUpdateMonitor.CreateGPUSceneBufferUpdateRenderPass(builder);

				constexpr GPUSceneBufferID NEXT_ID = static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) + 1);
				self.operator()<NEXT_ID>();
			}
		};
		createRenderPassesLambda.operator()<static_cast<GPUSceneBufferID>(0)>();

		mControlBlock.store(UpdateControlBlock{
			.NumThreadsUpdating = 0,
			.BlockUpdates = false
		}, std::memory_order::relaxed);

		return renderPassTuple;
	}
}