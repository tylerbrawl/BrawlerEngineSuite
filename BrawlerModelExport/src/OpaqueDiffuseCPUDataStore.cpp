module;
#include <cassert>
#include <span>
#include <DxDef.h>

module Brawler.OpaqueDiffuseModelTextureResolver;

namespace Brawler
{
	void OpaqueDiffuseModelTextureResolver::AddCPUDataStoreCopyRenderPasses(TextureResolutionContext& context)
	{
		assert(!context.VirtualTexturePageArr.empty());

		mTextureDataStore.CreateCPUPageStores(std::span<VirtualTexturePage>{ context.VirtualTexturePageArr }, context.Builder);

		/*
		// STEP 4
		//
		// Copy the generated textures into a persistent BufferResource in a READBACK heap.
		assert(!context.HBC7TextureDataReservationArr.empty());

		mBC7BufferSubAllocationArr.reserve(context.HBC7TextureDataReservationArr.size());

		std::size_t requiredReadBackBufferSize = 0;

		// Add the sizes of each BufferSubAllocationReservation returned as a result of BC7 image
		// compression to the size of the read-back buffer. We also need to account for the alignment
		// of StructuredBufferSubAllocation<BufferBC7> instances, since these are what will be
		// written into.

		constexpr std::size_t REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT = sizeof(BufferBC7);

		for (const auto& hBC7SubResourceReservation : context.HBC7TextureDataReservationArr)
		{
			// We want to ensure that we can create another StructuredBufferSubAllocation<BufferBC7> for
			// hBC7SubResourceReservation. To do that, add padding to the required buffer size so that
			// the sub-allocation can be properly aligned.

			if constexpr (Util::Math::IsPowerOfTwo(REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT))
				Util::Math::AlignToPowerOfTwo(requiredReadBackBufferSize, REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT);

			else
				Util::Math::Align(requiredReadBackBufferSize, REQUIRED_BUFFER_BC7_SUB_ALLOCATION_ALIGNMENT);

			requiredReadBackBufferSize += hBC7SubResourceReservation->GetReservationSize();
		}

		D3D12::BufferResourceInitializationInfo readBackBufferInitInfo{
			.SizeInBytes = requiredReadBackBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK
		};

		mOutputBuffer = std::make_unique<D3D12::BufferResource>(readBackBufferInitInfo);

		struct BC7DataCopyInfo
		{
			D3D12::StructuredBufferSubAllocation<BufferBC7>& DestSubAllocation;
			D3D12::StructuredBufferSubAllocation<BufferBC7> SrcSubAllocation;
		};

		for (auto&& hBC7SubResourceReservation : context.HBC7TextureDataReservationArr)
		{
			const std::size_t numBC7BlocksInReservation = (hBC7SubResourceReservation->GetReservationSize() / sizeof(BufferBC7));
			
			std::optional<D3D12::StructuredBufferSubAllocation<BufferBC7>> bc7BufferSubAllocation{ mOutputBuffer->CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<BufferBC7>>(numBC7BlocksInReservation) };
			assert(bc7BufferSubAllocation.has_value());

			mBC7BufferSubAllocationArr.push_back(std::move(*bc7BufferSubAllocation));

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, BC7DataCopyInfo> bc7SubResourceCopyPass{};
			bc7SubResourceCopyPass.SetRenderPassName("Opaque Diffuse Model Texture Resolver - Read-Back Buffer BC7 Texture Sub-Resource Copy");

			BC7DataCopyInfo copyInfo{
				.DestSubAllocation{mBC7BufferSubAllocationArr.back()},
				.SrcSubAllocation{ numBC7BlocksInReservation }
			};

			assert(copyInfo.SrcSubAllocation.IsReservationCompatible(hBC7SubResourceReservation));
			copyInfo.SrcSubAllocation.AssignReservation(std::move(hBC7SubResourceReservation));

			bc7SubResourceCopyPass.AddResourceDependency(copyInfo.DestSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			bc7SubResourceCopyPass.AddResourceDependency(copyInfo.SrcSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			bc7SubResourceCopyPass.SetInputData(std::move(copyInfo));

			bc7SubResourceCopyPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const BC7DataCopyInfo& copyInfo)
			{
				const D3D12::StructuredBufferSnapshot<BufferBC7> destSnapshot{ copyInfo.DestSubAllocation };
				const D3D12::StructuredBufferSnapshot<BufferBC7> srcSnapshot{ copyInfo.SrcSubAllocation };
				
				context.CopyBufferToBuffer(destSnapshot, srcSnapshot);
			});

			D3D12::RenderPassBundle bc7SubResourceCopyBundle{};
			bc7SubResourceCopyBundle.AddRenderPass(std::move(bc7SubResourceCopyPass));

			context.Builder.AddRenderPassBundle(std::move(bc7SubResourceCopyBundle));
		}
		*/
	}
}