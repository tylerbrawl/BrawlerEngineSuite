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
		/// <summary>
		/// This is a map between offsets from the start of the GPU scene buffer and a
		/// GPUSceneBufferUpdateOperation for the next frame. Since the object is expected to be accessed
		/// concurrently by multiple threads, it is thread safe. In fact, the actual map itself is
		/// lock free; the std::mutex in the template only protects the individual data elements contained
		/// within the map. However, it is unlikely that two threads will ever access the same element.
		/// </summary>
		ThreadSafeUnorderedMap<std::size_t, GPUSceneBufferUpdateOperation<BufferID>, std::mutex> mCurrUpdateRegionsMap;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	void GPUSceneBufferUpdateMonitor<BufferID>::UpdateBufferElement(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation)
	{
		const std::size_t offsetFromBufferStart = updateOperation.GetDestinationCopyRegion().GetOffsetFromBufferStart();

		// Unlike std::unordered_map::try_emplace, Brawler::ThreadSafeUnorderedMap::TryEmplace()
		// will immediately do a std::move() if possible, even if the data does not end up actually
		// getting constructed. To prevent the case where the data is moved into a node which never
		// actually gets added to the map, we first call TryEmplace() with no additional arguments
		// for the Value; this should be fine, since all of the types stored in GPU scene buffers
		// should trivially be default constructible.
		if (!mCurrUpdateRegionsMap.Contains(offsetFromBufferStart))
			mCurrUpdateRegionsMap.TryEmplace(offsetFromBufferStart);

		mCurrUpdateRegionsMap.AccessData(offsetFromBufferStart, [&updateOperation] (GPUSceneBufferUpdateOperation<BufferID>& storedUpdateOperation)
		{
			storedUpdateOperation = std::move(updateOperation);
		});
	}

	template <GPUSceneBufferID BufferID>
	GPUSceneBufferUpdateMonitor<BufferID>::BufferUpdateRenderPass_T GPUSceneBufferUpdateMonitor<BufferID>::CreateGPUSceneBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<BufferUpdateInfo> bufferUpdateInfoArr{};

		mCurrUpdateRegionsMap.ForEach([&bufferUpdateInfoArr] (GPUSceneBufferUpdateOperation<BufferID>& updateOperation)
		{
			bufferUpdateInfoArr.push_back(BufferUpdateInfo{
				.UpdateOperation{ std::move(updateOperation) }
			});
		});

		mCurrUpdateRegionsMap.Clear();

		const std::size_t requiredBufferSize = (sizeof(ElementType) * bufferUpdateInfoArr.size());
		D3D12::BufferResource& uploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		std::optional<D3D12::DynamicByteAddressBufferSubAllocation> uploadDataSubAllocation{ uploadBuffer.CreateBufferSubAllocation<D3D12::DynamicByteAddressBufferSubAllocation>(requiredBufferSize) };
		assert(uploadDataSubAllocation.has_value());

		for (const auto i : std::views::iota(0ull, bufferUpdateInfoArr.size()))
		{
			const std::span<const std::byte> srcDataByteSpan{ std::as_bytes(std::span<const ElementType>{ &(bufferUpdateInfoArr[i].UpdateOperation.GetUpdateSourceData()), 1 }) };
			const std::size_t offsetFromSubAllocationStart = (sizeof(ElementType) * i);

			uploadDataSubAllocation->WriteRawBytesToBuffer(srcDataByteSpan, offsetFromSubAllocationStart);
			bufferUpdateInfoArr[i].SourceCopyRegion = uploadDataSubAllocation->GetBufferCopyRegion(offsetFromSubAllocationStart, sizeof(ElementType));
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
		
		return !mCurrUpdateRegionsMap.Empty();
	}
}