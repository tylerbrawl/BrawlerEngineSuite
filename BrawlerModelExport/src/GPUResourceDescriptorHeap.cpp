module;
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceDescriptors.GPUResourceDescriptorHeap;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceDescriptors.DescriptorTableBuilder;

namespace Brawler
{
	namespace D3D12
	{
		__forceinline std::uint32_t GPUResourceDescriptorHeap::GetBasePerFrameDescriptorHeapIndex() const
		{
			// The entire range of indices for all per-frame descriptors in the descriptor heap is
			// [BINDLESS_SRVS_PARTITION_SIZE, (RESOURCE_DESCRIPTOR_HEAP_SIZE) - 1] (or
			// [500000, 999999]).
			//
			// The base index for descriptors allocated on even frames is BINDLESS_SRVS_PARTITION_SIZE.
			// The base index for descriptors allocated on odd frames is (PER_FRAME_DESCRIPTORS_PARTITION_SIZE)
			// indices after that.
			//
			// We could implement this with a branch, but this is probably more performant. (It
			// would also be the right way to do this if we were working with HLSL.)
			return static_cast<std::uint32_t>(BINDLESS_SRVS_PARTITION_SIZE + ((PER_FRAME_DESCRIPTORS_PARTITION_SIZE / 2) * (Util::Engine::GetCurrentFrameNumber() % 2)));
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceDescriptorHeap::Initialize()
		{
			// Create the shader-visible resource descriptor heap.
			{
				const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
					.NumDescriptors = Brawler::D3D12::RESOURCE_DESCRIPTOR_HEAP_SIZE,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					.NodeMask = 0
				};

				CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap)));
			}

			// Create a separate index for every available bindless SRV index.
			{
				for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(BINDLESS_SRVS_PARTITION_SIZE); ++i)
				{
					// We can ignore the warning because we know that we are only
					// pushing back as much as the queue can hold.
					std::ignore = mBindlessIndexQueue.PushBack(i);
				}
			}
		}

		BindlessSRVAllocation GPUResourceDescriptorHeap::CreateBindlessSRV(I_GPUResource& resource)
		{
			const std::optional<std::uint32_t> extractedIndex{ mBindlessIndexQueue.TryPop() };
			assert(extractedIndex.has_value() && "ERROR: The GPUResourceDescriptorHeap ran out of bindless SRV indices!");

			const std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc{ resource.CreateSRVDescription() };
			assert(srvDesc.has_value() && "ERROR: An attempt was made to create a bindless SRV for an I_GPUResource, but it did not provide a valid D3D12_SHADER_RESOURCE_VIEW_DESC!");

			const CD3DX12_CPU_DESCRIPTOR_HANDLE hAllocatedIndexHandle{ GetCPUDescriptorHandle(*extractedIndex) };

			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(resource.GetD3D12Resource()), &(*srvDesc), hAllocatedIndexHandle);

			return BindlessSRVAllocation{ *extractedIndex };
		}

		void GPUResourceDescriptorHeap::ReClaimBindlessSRV(BindlessSRVAllocation& srvAllocation)
		{
			std::ignore = mBindlessIndexQueue.PushBack(srvAllocation.GetBindlessSRVIndex());
			srvAllocation.ResetAllocationIndex();
		}

		PerFrameDescriptorTable GPUResourceDescriptorHeap::CreatePerFrameDescriptorTable(const DescriptorTableBuilder& tableBuilder)
		{
			const DescriptorHandleInfo handleInfo{ CreatePerFrameDescriptorHeapReservation(tableBuilder.GetDescriptorTableSize()) };
			
			// Copy the descriptors from the DescriptorTableBuilder's staging descriptor heap to the shader-visible
			// descriptor heap owned by this GPUResourceDescriptorHeap.
			Util::Engine::GetD3D12Device().CopyDescriptorsSimple(
				tableBuilder.GetDescriptorTableSize(),
				handleInfo.HCPUDescriptor,
				tableBuilder.GetCPUDescriptorHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);

			return PerFrameDescriptorTable{ PerFrameDescriptorTable::InitializationInfo{
				.HandleInfo{handleInfo},
				.CurrentFrameNumber = Util::Engine::GetCurrentFrameNumber()
			} };
		}

		void GPUResourceDescriptorHeap::ResetPerFrameDescriptorHeapIndex()
		{
			mPerFrameIndex.store(0);
		}

		DescriptorHandleInfo GPUResourceDescriptorHeap::CreatePerFrameDescriptorHeapReservation(const std::uint32_t numDescriptors)
		{
			// Reserve numDescriptors descriptors in the per-frame segment of the descriptor heap.
			const std::uint32_t frameSegmentIndexModifier = mPerFrameIndex.fetch_add(numDescriptors);
			assert(frameSegmentIndexModifier < PER_FRAME_DESCRIPTORS_PARTITION_SIZE && "ERROR: The limit of 250,000 per-frame descriptors has been exceeded!");

			const std::uint32_t descriptorHeapIndex = GetBasePerFrameDescriptorHeapIndex() + frameSegmentIndexModifier;

			return DescriptorHandleInfo{
				.HCPUDescriptor{ mHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(descriptorHeapIndex), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) },
				.HGPUDescriptor{ mHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(descriptorHeapIndex), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) }
			};
		}
	}
}