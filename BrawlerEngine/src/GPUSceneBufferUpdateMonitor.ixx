module;
#include <cstdint>
#include <mutex>
#include <vector>
#include <optional>
#include <cassert>
#include <span>
#include <ranges>
#include <DxDef.h>

export module Brawler.GPUSceneBufferUpdateSubModule:GPUSceneBufferUpdateMonitor;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBufferMap;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.ThreadSafeUnorderedMap;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.ByteAddressBufferSubAllocation;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.GPUSceneManager;
import Brawler.D3D12.GPUCommandContexts;

export namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	class GPUSceneBufferUpdateMonitor
	{
	private:
		using ElementType = GPUSceneBufferElementType<BufferID>;

	private:
		struct BufferUpdateInfo
		{
			GPUSceneBufferUpdateOperation<BufferID> UpdateOperation;
			D3D12::BufferCopyRegion SourceCopyRegion;
		};

		struct BufferUpdatePassInfo
		{
			std::vector<BufferUpdateInfo> UpdateInfoArr;
		};

	public:
		using BufferUpdateRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, BufferUpdatePassInfo>;

	public:
		GPUSceneBufferUpdateMonitor() = default;

		GPUSceneBufferUpdateMonitor(const GPUSceneBufferUpdateMonitor& rhs) = delete;
		GPUSceneBufferUpdateMonitor& operator=(const GPUSceneBufferUpdateMonitor& rhs) = delete;

		GPUSceneBufferUpdateMonitor(GPUSceneBufferUpdateMonitor&& rhs) noexcept = default;
		GPUSceneBufferUpdateMonitor& operator=(GPUSceneBufferUpdateMonitor&& rhs) noexcept = default;

		void UpdateBufferElement(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation);
		BufferUpdateRenderPass_T CreateGPUSceneBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder);

		bool HasScheduledBufferUpdates() const;

	private:
		std::vector<GPUSceneBufferUpdateOperation<BufferID>> mUpdateOperationArr;
		mutable std::mutex mCritSection;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	void GPUSceneBufferUpdateMonitor<BufferID>::UpdateBufferElement(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation)
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };

		mUpdateOperationArr.push_back(std::move(updateOperation));
	}

	template <GPUSceneBufferID BufferID>
	GPUSceneBufferUpdateMonitor<BufferID>::BufferUpdateRenderPass_T GPUSceneBufferUpdateMonitor<BufferID>::CreateGPUSceneBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<BufferUpdateInfo> bufferUpdateInfoArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };

			for (auto&& updateOperation : mUpdateOperationArr)
			{
				bufferUpdateInfoArr.push_back(BufferUpdateInfo{
					.UpdateOperation{ std::move(updateOperation) }
				});
			}

			mUpdateOperationArr.clear();
		}

		const std::size_t requiredBufferSize = (sizeof(ElementType) * bufferUpdateInfoArr.size());
		D3D12::BufferResource& uploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		std::optional<D3D12::DynamicByteAddressBufferSubAllocation> uploadDataSubAllocation{ uploadBuffer.CreateBufferSubAllocation<D3D12::DynamicByteAddressBufferSubAllocation>(requiredBufferSize) };
		assert(uploadDataSubAllocation.has_value());

		std::size_t currOffsetFromSubAllocationStart = 0;
		for (auto& updateInfo : bufferUpdateInfoArr)
		{
			const std::span<const std::byte> srcDataByteSpan{ std::as_bytes(updateInfo.UpdateOperation.GetUpdateSourceDataSpan()) };

			uploadDataSubAllocation->WriteRawBytesToBuffer(srcDataByteSpan, currOffsetFromSubAllocationStart);
			bufferUpdateInfoArr[i].SourceCopyRegion = uploadDataSubAllocation->GetBufferCopyRegion(currOffsetFromSubAllocationStart, srcDataByteSpan.size_bytes());

			currOffsetFromSubAllocationStart += srcDataByteSpan.size_bytes();
		}

		BufferUpdateRenderPass_T updatePass{};
		updatePass.SetRenderPassName("GPU Scene Buffer Update Pass");

		updatePass.AddResourceDependency(D3D12::FrameGraphResourceDependency{
			.ResourcePtr = &(GPUSceneManager::GetInstance().GetGPUSceneBufferResource<BufferID>()),
			.RequiredState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
			.SubResourceIndex = 0
		});
		updatePass.AddResourceDependency(*uploadDataSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		updatePass.SetInputData(BufferUpdatePassInfo{
			.UpdateInfoArr{ std::move(bufferUpdateInfoArr) }
		});

		updatePass.SetRenderPassCommands([] (D3D12::CopyContext& context, const BufferUpdatePassInfo& passInfo)
		{
			for (const auto& updateInfo : passInfo.UpdateInfoArr)
				context.CopyBufferToBuffer(updateInfo.UpdateOperation.GetDestinationCopyRegion(), updateInfo.SourceCopyRegion);
		});

		return updatePass;
	}

	template <GPUSceneBufferID BufferID>
	bool GPUSceneBufferUpdateMonitor<BufferID>::HasScheduledBufferUpdates() const
	{
		// Strictly speaking, this function is obviously very prone to race conditions, but I
		// argue that they are ultimately benign. If a calling class needs to guarantee that
		// an update is done during a specific frame, then there are ways to do this in the
		// Brawler Engine. For instance, FrameGraph building for a given frame does not happen
		// until any updates to the SceneGraph have been completed.
		//
		// So, if there is a need for an update to happen on a specific frame, then we leave it
		// up to other classes to ensure this. Otherwise, if a buffer update is scheduled immediately
		// after this function returns false, then it will still be applied on the next frame.
		
		const std::scoped_lock<std::mutex> lock{ mCritSection };

		return !mUpdateOperationArr.empty();
	}
}