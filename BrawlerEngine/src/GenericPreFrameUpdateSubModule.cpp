module;
#include <cassert>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <DxDef.h>

module Brawler.GenericPreFrameUpdateSubModule;
import Brawler.D3D12.FrameGraphResourceDependency;

namespace Brawler
{
	void GenericPreFrameUpdateSubModule::AddPreFrameBufferUpdate(GenericPreFrameBufferUpdate&& preFrameUpdate)
	{
		const std::scoped_lock<std::mutex> lock{ mBufferUpdateCritSection };
		mPendingBufferUpdateArr.push_back(std::move(preFrameUpdate));
	}

	bool GenericPreFrameUpdateSubModule::HasPendingUpdates() const
	{
		const std::scoped_lock<std::mutex> lock{ mBufferUpdateCritSection };
		return !mPendingBufferUpdateArr.empty();
	}

	GenericPreFrameUpdateSubModule::BufferUpdateRenderPass_T GenericPreFrameUpdateSubModule::CreateBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<GenericPreFrameBufferUpdate> extractedUpdateArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mBufferUpdateCritSection };
			extractedUpdateArr = std::move(mPendingBufferUpdateArr);
		}

		// This function shouldn't be called if GenericPreFrameUpdateSubModule::HasPendingUpdates()
		// would have returned false.
		assert(!extractedUpdateArr.empty());

		// Create an UPLOAD buffer which contains all of the data which must be submitted to
		// the GPU.
		std::size_t uploadBufferSize = 0;
		
		for (const auto& update : extractedUpdateArr)
			uploadBufferSize += update.GetUpdateRegionSize();

		D3D12::BufferResource& uploadBufferResource{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = uploadBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		BufferUpdateRenderPass_T updatePass{};
		updatePass.SetRenderPassName("Generic Pre-Frame Buffer Update Pass");

		updatePass.AddResourceDependency(D3D12::FrameGraphResourceDependency{
			.ResourcePtr = &uploadBufferResource,
			.RequiredState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
			.SubResourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
		});

		std::vector<BufferUpdateOperation> updateOperationArr{};
		updateOperationArr.reserve(extractedUpdateArr.size());

		// We aren't allowed to add a sub-resource as a resource dependency multiple times
		// to a single RenderPass instance, so we keep a std::unordered_set to track which
		// ones we still need to add.
		std::unordered_set<D3D12::BufferResource*> addedBufferPtrDependencySet{};

		for (auto& update : extractedUpdateArr)
		{
			BufferUpdateOperation currUpdateOperation{
				.DestCopyRegion{ update.GetBufferCopyRegion() },
				.SrcBufferSubAllocation{ update.GetUpdateRegionSize() }
			};

			[[maybe_unused]] const bool wasReservationSuccessful = uploadBufferResource.AssignReservation(currUpdateOperation.SrcBufferSubAllocation);
			assert(wasReservationSuccessful);

			// Copy the data into the UPLOAD buffer.
			currUpdateOperation.SrcBufferSubAllocation.WriteRawBytesToBuffer(update.GetDataByteSpan());

			D3D12::BufferResource* const destBufferResourcePtr = &(update.GetBufferResource());

			if (!addedBufferPtrDependencySet.contains(destBufferResourcePtr)) [[likely]]
			{
				updatePass.AddResourceDependency(D3D12::FrameGraphResourceDependency{
					.ResourcePtr = destBufferResourcePtr,
					.RequiredState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST,
					.SubResourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
				});

				addedBufferPtrDependencySet.insert(destBufferResourcePtr);
			}

			updateOperationArr.push_back(std::move(currUpdateOperation));
		}

		updatePass.SetInputData(std::move(updateOperationArr));

		updatePass.SetRenderPassCommands([] (D3D12::CopyContext& context, const BufferUpdatePassInfo& passInfo)
		{
			for (const auto& updateOperation : passInfo)
				context.CopyBufferToBuffer(passInfo.DestCopyRegion, passInfo.SrcBufferSubAllocation.GetBufferCopyRegion());
		});

		return updatePass;
	}
}