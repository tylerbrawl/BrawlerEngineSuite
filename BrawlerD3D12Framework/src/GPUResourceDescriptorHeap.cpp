module;
#include <cassert>
#include <optional>
#include <mutex>
#include <memory>
#include <array>
#include <atomic>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.DescriptorTableBuilder;
import Util.Engine;
import Util.General;

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
		void GPUResourceDescriptorHeap::InitializeBindlessSRVQueue()
		{
			// Create a separate index for every available bindless SRV index. This is done
			// separately from initializing the ID3D12DescriptorHeap because it is a long-running
			// process.
			//
			// Although this involves a lot of heap allocations, the process is done concurrently
			// with D3D12 device creation. Since that process is unavoidable and takes a long-ass
			// time anyways, doing this in parallel allows the queue initialization to essentially
			// become "free."
			{
				std::scoped_lock<std::mutex> lock{ mBindlessIndexQueue.CritSection };

				for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(BINDLESS_SRVS_PARTITION_SIZE); ++i)
					mBindlessIndexQueue.Queue.push(i);
			}
		}

		void GPUResourceDescriptorHeap::InitializeD3D12DescriptorHeap()
		{
			// Create the shader-visible resource descriptor heap.
			{
				const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
					.NumDescriptors = Brawler::D3D12::RESOURCE_DESCRIPTOR_HEAP_SIZE,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					.NodeMask = 0
				};

				Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap)));
			}

			// Cache the descriptor handle increment size.
			mDescriptorHandleIncrementSize = Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		std::unique_ptr<BindlessSRVSentinel> GPUResourceDescriptorHeap::AllocateBindlessSRV()
		{
			std::uint32_t extractedIndex = 0;

			{
				std::scoped_lock<std::mutex> lock{ mBindlessIndexQueue.CritSection };

				assert(!mBindlessIndexQueue.Queue.empty() && "ERROR: The limit of 500,000 bindless SRVs has been exceeded!");

				extractedIndex = mBindlessIndexQueue.Queue.front();
				mBindlessIndexQueue.Queue.pop();
			}

			BindlessSRVSentinel bindlessSentinel{ extractedIndex };
			return std::make_unique<BindlessSRVSentinel>(std::move(bindlessSentinel));
		}

		void GPUResourceDescriptorHeap::ReClaimBindlessSRV(BindlessSRVSentinel& srvAllocation)
		{
			std::scoped_lock<std::mutex> lock{ mBindlessIndexQueue.CritSection };

			mBindlessIndexQueue.Queue.push(srvAllocation.GetBindlessSRVIndex());
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
			mPerFrameIndexArr[Util::Engine::GetTrueFrameNumber()].store(0, std::memory_order::relaxed);
		}

		Brawler::D3D12DescriptorHeap& GPUResourceDescriptorHeap::GetD3D12DescriptorHeap() const
		{
			assert(mHeap != nullptr && "ERROR: An attempt was made to get the ID3D12DescriptorHeap* of the GPUResourceDescriptorHeap before it could ever be created!");
			return *(mHeap.Get());
		}

		DescriptorHandleInfo GPUResourceDescriptorHeap::CreatePerFrameDescriptorHeapReservation(const std::uint32_t numDescriptors)
		{
			// Reserve numDescriptors descriptors in the per-frame segment of the descriptor heap.
			const std::uint32_t frameSegmentIndexModifier = mPerFrameIndexArr[Util::Engine::GetCurrentFrameNumber()].fetch_add(numDescriptors, std::memory_order::relaxed);
			assert(frameSegmentIndexModifier < PER_FRAME_DESCRIPTORS_PARTITION_SIZE && "ERROR: The limit of 250,000 per-frame descriptors has been exceeded!");

			const std::uint32_t descriptorHeapIndex = GetBasePerFrameDescriptorHeapIndex() + frameSegmentIndexModifier;

			return DescriptorHandleInfo{
				.HCPUDescriptor{ mHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(descriptorHeapIndex), mDescriptorHandleIncrementSize },
				.HGPUDescriptor{ mHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(descriptorHeapIndex), mDescriptorHandleIncrementSize }
			};
		}
	}
}