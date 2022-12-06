module;
#include <cassert>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ranges>
#include <algorithm>
#include <DxDef.h>

module Brawler.GenericPreFrameUpdateSubModule;
import Brawler.D3D12.FrameGraphResourceDependency;
import Util.Math;

namespace Brawler
{
	void GenericPreFrameUpdateSubModule::AddPreFrameBufferUpdate(GenericPreFrameBufferUpdate&& preFrameUpdate)
	{
		const std::scoped_lock<std::mutex> lock{ mBufferUpdateCritSection };
		mPendingBufferUpdateArr.push_back(std::move(preFrameUpdate));
	}

	bool GenericPreFrameUpdateSubModule::HasPendingBufferUpdates() const
	{
		const std::scoped_lock<std::mutex> lock{ mBufferUpdateCritSection };
		return !mPendingBufferUpdateArr.empty();
	}

	void GenericPreFrameUpdateSubModule::AddPreFrameTextureUpdate(GenericPreFrameTextureUpdate&& preFrameUpdate)
	{
		const std::scoped_lock<std::mutex> lock{ mTextureUpdateCritSection };
		mPendingTextureUpdateArr.push_back(std::move(preFrameUpdate));
	}

	bool GenericPreFrameUpdateSubModule::HasPendingTextureUpdates() const
	{
		const std::scoped_lock<std::mutex> lock{ mTextureUpdateCritSection };
		return !mPendingTextureUpdateArr.empty();
	}

	GenericPreFrameUpdateSubModule::BufferUpdateRenderPass_T GenericPreFrameUpdateSubModule::CreateBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<GenericPreFrameBufferUpdate> extractedUpdateArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mBufferUpdateCritSection };
			extractedUpdateArr = std::move(mPendingBufferUpdateArr);
		}

		// This function shouldn't be called if GenericPreFrameUpdateSubModule::HasPendingBufferUpdates()
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

	GenericPreFrameUpdateSubModule::TextureUpdateRenderPass_T GenericPreFrameUpdateSubModule::CreateTextureUpdateRenderPass(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<GenericPreFrameTextureUpdate> extractedUpdateArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mTextureUpdateCritSection };
			extractedUpdateArr = std::move(mPendingTextureUpdateArr);
		}

		// This function shouldn't be called if GenericPreFrameUpdateSubModule::HasPendingTextureUpdates()
		// returned false.
		assert(!extractedUpdateArr.empty());

		// Get the required size of the UPLOAD buffer. We need to also account for the fact that
		// texture data itself must be at an aligned offset from the start of the buffer
		// (specifically, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT).
		std::size_t requiredUploadBufferSize = extractedUpdateArr.front().GetUpdateRegionSize();

		for (const auto& extractedUpdate : extractedUpdateArr | std::views::drop(1))
			requiredUploadBufferSize = (Util::Math::AlignToPowerOfTwo(requiredUploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) + extractedUpdate.GetUpdateRegionSize());

		D3D12::BufferResource& uploadBufferResource{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredUploadBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		std::vector<TextureUpdateOperation> updateOperationArr{};
		updateOperationArr.reserve(extractedUpdateArr.size());

		TextureUpdateRenderPass_T textureUpdateRenderPass{};
		textureUpdateRenderPass.SetRenderPassName("Generic Pre-Frame Texture Update Pass");

		textureUpdateRenderPass.AddResourceDependency(D3D12::FrameGraphResourceDependency{
			.ResourcePtr = &uploadBufferResource,
			.RequiredState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
			.SubResourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
		});

		// We aren't allowed to specify the same sub-resource in a RenderPass as a resource dependency
		// more than once, so we keep a map between resources and the sub-resource indices for which a
		// resource dependency has been created.
		std::unordered_map<I_GPUResource*, std::vector<std::uint32_t>> addedResourceDependenciesMap{};

		for (auto& extractedUpdate : extractedUpdateArr)
		{
			// Add the sub-resource specified in extractedUpdate as a resource dependency if it wasn't
			// added already.
			{
				D3D12::TextureSubResource& destTextureSubResource{ extractedUpdate.GetDestinationTextureSubResource() };
				I_GPUResource* const destTextureResourcePtr = &(destTextureSubResource.GetGPUResource());

				if (!addedResourceDependenciesMap.contains(destTextureResourcePtr))
				{
					// If the I_GPUResource* wasn't already in the map, then we obviously haven't added this
					// sub-resource as a dependency yet, so we do that now.
					textureUpdateRenderPass.AddResourceDependency(destTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
					addedResourceDependenciesMap[destTextureResourcePtr].push_back(destTextureSubResource.GetSubResourceIndex());
				}
				else
				{
					std::vector<std::uint32_t>& currSubResourceIndicesArr{ addedResourceDependenciesMap.at(destTextureResourcePtr) };
					const auto itr = std::ranges::find(currSubResourceIndicesArr, destTextureSubResource.GetSubResourceIndex());

					if (itr == currSubResourceIndicesArr.end())
					{
						textureUpdateRenderPass.AddResourceDependency(destTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
						currSubResourceIndicesArr.push_back(destTextureSubResource.GetSubResourceIndex());
					}
				}
			}
			
			D3D12::TextureCopyBufferSubAllocation currSubAllocation{ extractedUpdate.GetTextureCopyRegion() };

			{
				[[maybe_unused]] const bool wasReservationSuccessful = uploadBufferResource.AssignReservation(currSubAllocation);
				assert(wasReservationSuccessful);
			}

			// We assume that the data specified in extractedUpdate.GetDataByteSpan() is already properly
			// sized and aligned for texture copy operations. So, instead of copying the texture data
			// one row at a time, we use I_BufferSubAllocation::WriteToBuffer() to copy everything at
			// once.
			currSubAllocation.WriteToBuffer(extractedUpdate.GetDataByteSpan(), 0);

			updateOperationArr.push_back(TextureUpdateOperation{
				.DestCopyRegion{ extractedUpdate.GetTextureCopyRegion() },
				.SrcBufferSubAllocation{ std::move(currSubAllocation) }
			});
		}

		textureUpdateRenderPass.SetInputData(std::move(updateOperationArr));

		textureUpdateRenderPass.SetRenderPassCommands([] (D3D12::CopyContext& context, const TextureUpdatePassInfo& passInfo)
		{
			for (const auto& updateOperation : passInfo)
			{
				const D3D12::TextureCopyBufferSnapshot srcBufferSnapshot{ passInfo.SrcBufferSubAllocation };
				context.CopyBufferToTexture(passInfo.DestCopyRegion, srcBufferSnapshot);
			}
		});

		return textureUpdateRenderPass;
	}
}