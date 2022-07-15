module;
#include <mutex>
#include <span>
#include <vector>
#include <memory>
#include <optional>
#include <cassert>
#include <DirectXMath/DirectXMath.h>

module Brawler.GPUSceneUpdateRenderModule;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.ByteAddressBufferSubAllocation;
import Brawler.D3D12.FrameGraphResourceDependency;

namespace Brawler
{
	void GPUSceneUpdateRenderModule::ScheduleGPUSceneBufferUpdateForNextFrame(const I_GPUSceneBufferUpdateSource& bufferUpdateSource)
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		mBufferUpdateSrcPtrSet.insert(&bufferUpdateSource);
	}

	bool GPUSceneUpdateRenderModule::IsRenderModuleEnabled() const
	{
		// Don't bother trying to create RenderPass instances for GPU scene updates if there are
		// no updates to perform this frame.
		
		std::scoped_lock<std::mutex> lock{ mCritSection };

		return !mBufferUpdateSrcPtrSet.empty();
	}

	void GPUSceneUpdateRenderModule::BuildFrameGraph(D3D12::FrameGraphBuilder& builder)
	{
		UpdateGPUSceneBuffers(builder);
	}

	void GPUSceneUpdateRenderModule::UpdateGPUSceneBuffers(D3D12::FrameGraphBuilder& builder)
	{
		/*
		Updating GPU Scene Buffers
		---------------------------

		To implement GPU scene buffer updates, we rely on the fact that scene graph updates cannot
		overlap with frame graph building. (They can, however, overlap with command list recording.)

		During frame graph building, each I_GPUSceneBufferUpdateSource instance which reported that
		it needs to update GPU scene buffer data for the next frame has its data copied into a transient
		buffer resource located in an UPLOAD heap. (Technically, it actually gets written to a temporary
		CPU data store and is later automatically written to the GPU. This is done because the ID3D12Resource
		for the transient buffer resource won't be available until later.)

		During command list recording, the data is then copied from the upload buffer into the specified
		GPU scene buffer regions on the GPU timeline. Since this copy is (and must) be done on the GPU
		timeline, we guarantee that we modify the data only after the previous frame's commands have stopped
		reading it.
		*/

		struct GPUSceneBufferUpdateInfo
		{
			D3D12::BufferResource* DestGPUSceneBufferPtr;
			std::span<const std::byte> UploadSrcDataSpan;
			const D3D12::BufferCopyRegion* UploadDestRegionPtr;
		};

		std::vector<GPUSceneBufferUpdateInfo> updateInfoArr{};

		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			updateInfoArr.reserve(mBufferUpdateSrcPtrSet.size());

			for (const auto updateSrcPtr : mBufferUpdateSrcPtrSet)
				updateInfoArr.emplace_back(&(updateSrcPtr->GetGPUSceneBufferResource()), updateSrcPtr->GetGPUSceneUploadData(), &(updateSrcPtr->GetGPUSceneBufferCopyDestination()));

			mBufferUpdateSrcPtrSet.clear();
		}

		// Create a transient upload BufferResource large enough to hold all of the data. The D3D12 API does
		// not require BufferCopyRegions to be aligned to a specific multiple of the start of the buffer
		// data, so we can tightly pack everything into a single buffer and use a ByteAddressBufferSubAllocation
		// to access the elements.
		std::size_t requiredBufferSize = 0;
		for (const auto& updateInfo : updateInfoArr)
			requiredBufferSize += updateInfo.UploadSrcDataSpan.size_bytes();

		D3D12::BufferResource& uploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		std::optional<D3D12::DynamicByteAddressBufferSubAllocation> byteAddressSubAllocation{ uploadBuffer.CreateBufferSubAllocation<D3D12::DynamicByteAddressBufferSubAllocation>(requiredBufferSize) };
		assert(byteAddressSubAllocation.has_value());

		struct GPUSceneBufferUpdateCopyRegions
		{
			const D3D12::BufferCopyRegion* UploadDestRegionPtr;
			D3D12::BufferCopyRegion UploadSrcRegion;
		};

		std::unordered_set<D3D12::BufferResource*> updatedGPUSceneBufferPtrSet{};
		std::vector<GPUSceneBufferUpdateCopyRegions> copyRegionsArr{};
		copyRegionsArr.reserve(updateInfoArr.size());
		
		{
			std::size_t currOffset = 0;

			for (const auto& updateInfo : updateInfoArr)
			{
				const std::size_t currDataSizeInBytes = updateInfo.UploadSrcDataSpan.size_bytes();

				// Write the data into the upload buffer immediately.
				byteAddressSubAllocation->WriteRawBytesToBuffer(updateInfo.UploadSrcDataSpan, currOffset);

				// Keep track of the GPU scene buffers which we are updating. We do this because we need to
				// transition them to the D3D12_RESOURCE_STATE_COPY_DEST state.
				updatedGPUSceneBufferPtrSet.insert(updateInfo.DestGPUSceneBufferPtr);

				// Create a D3D12::BufferCopyRegion instance for copying to the GPU scene buffer and associate
				// it with its corresponding destination D3D12::BufferCopyRegion.
				copyRegionsArr.emplace_back(updateInfo.UploadDestRegionPtr, byteAddressSubAllocation->GetBufferCopyRegion(currOffset, currDataSizeInBytes));

				// Adjust currOffset so that we can write the next data element to its designated location.
				currOffset += currDataSizeInBytes;
			}
		}

		struct GPUSceneBufferUpdatePassInfo
		{
			std::vector<GPUSceneBufferUpdateCopyRegions> CopyRegionsArr;
		};

		D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, GPUSceneBufferUpdatePassInfo> gpuSceneBufferUpdatePass{};
		gpuSceneBufferUpdatePass.SetRenderPassName("GPU Scene Representation Update - Buffer Data Update");

		for (const auto gpuSceneBufferPtr : updatedGPUSceneBufferPtrSet)
		{
			D3D12::FrameGraphResourceDependency resourceDependency{
				.ResourcePtr = gpuSceneBufferPtr,
				.RequiredState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
				.SubResourceIndex = 0
			};

			gpuSceneBufferUpdatePass.AddResourceDependency(std::move(resourceDependency));
		}

		gpuSceneBufferUpdatePass.AddResourceDependency(*byteAddressSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		gpuSceneBufferUpdatePass.SetInputData(GPUSceneBufferUpdatePassInfo{
			.CopyRegionsArr{ std::move(copyRegionsArr) }
		});

		gpuSceneBufferUpdatePass.SetRenderPassCommands([] (D3D12::CopyContext& context, const GPUSceneBufferUpdatePassInfo& passInfo)
		{
			for (const auto& copyRegionInfo : passInfo.CopyRegionsArr)
				context.CopyBufferToBuffer(*(copyRegionInfo.UploadDestRegionPtr), copyRegionInfo.UploadSrcRegion);
		});

		D3D12::RenderPassBundle gpuSceneBufferUpdatePassBundle{};
		gpuSceneBufferUpdatePassBundle.AddRenderPass(std::move(gpuSceneBufferUpdatePass));

		builder.AddRenderPassBundle(std::move(gpuSceneBufferUpdatePassBundle));
	}
}