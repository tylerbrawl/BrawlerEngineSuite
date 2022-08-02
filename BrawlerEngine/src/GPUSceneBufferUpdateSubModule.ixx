module;
#include <tuple>
#include <atomic>
#include <optional>

export module Brawler.GPUSceneBufferUpdateSubModule;
import :GPUSceneBufferUpdateMonitor;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferCopyRegion;
import Util.Coroutine;

namespace Brawler
{
	template <GPUSceneBufferID BufferID, typename... MonitorTypes>
	struct MonitorTupleSolver
	{
		using TupleType = typename MonitorTupleSolver<static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) - 1), GPUSceneBufferUpdateMonitor<BufferID>, MonitorTypes...>::TupleType;
	};

	template <typename... MonitorTypes>
	struct MonitorTupleSolver<static_cast<GPUSceneBufferID>(0), MonitorTypes...>
	{
		using TupleType = std::tuple<GPUSceneBufferUpdateMonitor<static_cast<GPUSceneBufferID>(0)>, MonitorTypes...>;
	};

	template <>
	struct MonitorTupleSolver<GPUSceneBufferID::COUNT_OR_ERROR>
	{
		using TupleType = typename MonitorTupleSolver<static_cast<GPUSceneBufferID>(std::to_underlying(GPUSceneBufferID::COUNT_OR_ERROR) - 1)>::TupleType;
	};

	using UpdateMonitorTuple = typename MonitorTupleSolver<GPUSceneBufferID::COUNT_OR_ERROR>::TupleType;

	template <typename T>
	struct UpdateRenderPassTupleSolver
	{};

	template <GPUSceneBufferID... BufferIDs>
	struct UpdateRenderPassTupleSolver<std::tuple<GPUSceneBufferUpdateMonitor<BufferIDs>...>>
	{
		using TupleType = std::tuple<std::optional<typename GPUSceneBufferUpdateMonitor<BufferIDs>::BufferUpdateRenderPass_T>...>;
	};

	using GPUSceneBufferUpdateRenderPassTuple = typename UpdateRenderPassTupleSolver<UpdateMonitorTuple>::TupleType;
}

export namespace Brawler
{
	class GPUSceneBufferUpdateSubModule
	{
	private:
		struct UpdateControlBlock
		{
			std::uint32_t NumThreadsUpdating;
			bool BlockUpdates;
		};

		// This isn't really necessary, but if it somehow returns false, then we should pack the
		// struct into a std::uint64_t in order to enforce lock freedom.
		static_assert(std::atomic<UpdateControlBlock>::is_always_lock_free);

	public:
		GPUSceneBufferUpdateSubModule() = default;

		GPUSceneBufferUpdateSubModule(const GPUSceneBufferUpdateSubModule& rhs) = delete;
		GPUSceneBufferUpdateSubModule& operator=(const GPUSceneBufferUpdateSubModule& rhs) = delete;

		GPUSceneBufferUpdateSubModule(GPUSceneBufferUpdateSubModule&& rhs) noexcept = default;
		GPUSceneBufferUpdateSubModule& operator=(GPUSceneBufferUpdateSubModule&& rhs) noexcept = default;

		template <GPUSceneBufferID BufferID>
		void ScheduleGPUSceneBufferUpdateForNextFrame(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation);

		bool HasScheduledBufferUpdates() const;
		GPUSceneBufferUpdateRenderPassTuple CreateGPUSceneBufferUpdateRenderPassTuple(D3D12::FrameGraphBuilder& builder);

	private:
		UpdateMonitorTuple mMonitorTuple;
		std::atomic<UpdateControlBlock> mControlBlock;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	void GPUSceneBufferUpdateSubModule::ScheduleGPUSceneBufferUpdateForNextFrame(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation)
	{
		UpdateControlBlock expectedControlBlock{ mControlBlock.load(std::memory_order::relaxed) };
		UpdateControlBlock desiredControlBlock{};

		do
		{
			if (expectedControlBlock.BlockUpdates) [[unlikely]]
			{
				// Just block until we can push out the update. In the meantime, we can go ahead and
				// execute other CPU jobs. (In fact, doing this might help us update the buffer sooner.)
				
				Util::Coroutine::TryExecuteJob();
				desiredControlBlock = expectedControlBlock;
			}
			else [[likely]]
			{
				desiredControlBlock = UpdateControlBlock{
					.NumThreadsUpdating = (expectedControlBlock.NumThreadsUpdating + 1),
					.BlockUpdates = false
				};
			}
		} while (!mControlBlock.compare_exchange_weak(expectedControlBlock, desiredControlBlock, std::memory_order::relaxed));

		std::get<GPUSceneBufferUpdateMonitor<BufferID>>(mMonitorTuple).UpdateBufferElement(std::move(updateOperation));

		expectedControlBlock = desiredControlBlock;

		do
		{
			desiredControlBlock = UpdateControlBlock{
				.NumThreadsUpdating = (expectedControlBlock.NumThreadsUpdating - 1),
				.BlockUpdates = expectedControlBlock.BlockUpdates
			};
		} while (!mControlBlock.compare_exchange_weak(expectedControlBlock, desiredControlBlock, std::memory_order::relaxed));

		mControlBlock.notify_one();
	}
}