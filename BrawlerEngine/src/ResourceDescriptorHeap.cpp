module;
#include <cstdint>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <span>
#include "DxDef.h"

module Brawler.ResourceDescriptorHeap;
import Util.Engine;
import Brawler.I_GPUResource;
import Brawler.ResourceDescriptorHeapAllocation;
import Brawler.DescriptorHandleInfo;
import Brawler.ResourceDescriptorTable;

namespace Brawler
{
	ResourceDescriptorHeap::ResourceDescriptorHeap() :
		mDescriptorHeap(nullptr),
		mFreeBindlessIndexQueue(),
		mPerFrameIndexArr(),
		mHandleIncrementSize(0)
	{
		for (auto& indexTracker : mPerFrameIndexArr)
			indexTracker.store(0);
	}

	void ResourceDescriptorHeap::Initialize()
	{
		CreateDescriptorHeap();
		InitializeFreeBindlessIndexQueue();

		mHandleIncrementSize = Util::Engine::GetD3D12Device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void ResourceDescriptorHeap::AdvanceFrame()
	{
		// Reset the per-frame index for the current frame.
		GetCurrentPerFrameIndexTracker().store(0);
	}

	void ResourceDescriptorHeap::CreateBindlessSRV(I_GPUResource& resource)
	{
		const std::optional<std::uint32_t> reservedIndex{ mFreeBindlessIndexQueue.TryPop() };
		if (!reservedIndex.has_value())
			throw std::runtime_error{ "ERROR: The capacity for bindless SRVs has been exceeded!" };
		
		ResourceDescriptorHeapAllocation allocation{ *reservedIndex };
		InitializeDescriptorHandlesForAllocation(allocation);

		Util::Engine::GetD3D12Device().CopyDescriptorsSimple(
			1,
			allocation.GetDescriptorHandles().CPUHandle,
			resource.GetCPUVisibleDescriptorHandle(ResourceDescriptorType::SRV),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

		resource.SetBindlessSRVAllocation(std::move(allocation));
	}

	void ResourceDescriptorHeap::CreatePerFrameDescriptorTable(ResourceDescriptorTable& descriptorTable)
	{
		const std::span<const ResourceDescriptorTable::DescriptorRange> descriptorRanges{ descriptorTable.GetDescriptorRanges() };

		// Find the total number of descriptors in the descriptor table and reserve space
		// in the current per-frame segment of the resource descriptor heap.
		std::uint32_t numDescriptors = 0;
		for (const auto& range : descriptorRanges)
			numDescriptors += static_cast<std::uint32_t>(range.ResourcePtrArr.size());

		const std::uint32_t beginIndex = CreatePerFrameReservation(numDescriptors);

		// For each descriptor range, copy each I_GPUResource's CPU-visible descriptor
		// into the GPU-visible descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE hResourceHeapCPUHandle{ mDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		hResourceHeapCPUHandle.Offset(beginIndex, mHandleIncrementSize);

		for (const auto& range : descriptorRanges)
		{
			for (const auto resourcePtr : range.ResourcePtrArr)
			{
				Util::Engine::GetD3D12Device().CopyDescriptorsSimple(
					1,
					hResourceHeapCPUHandle,
					resourcePtr->GetCPUVisibleDescriptorHandle(range.RootParamType),
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
				);

				hResourceHeapCPUHandle.Offset(1, mHandleIncrementSize);
			}
		}

		// Notify the ResourceDescriptorTable about where its allocation into the
		// GPU-visible resource descriptor heap starts.
		DescriptorHandleInfo tableStartInfo{
			.CPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE{ mDescriptorHeap->GetCPUDescriptorHandleForHeapStart() }.Offset(beginIndex, mHandleIncrementSize),
			.GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE{ mDescriptorHeap->GetGPUDescriptorHandleForHeapStart() }.Offset(beginIndex, mHandleIncrementSize)
		};
		descriptorTable.SetDescriptorHandlesForTableStart(std::move(tableStartInfo));
	}

	void ResourceDescriptorHeap::DeleteAllocation(ResourceDescriptorHeapAllocation&& allocation)
	{
		if (!allocation.IsAllocationValid())
			return;
		
		assert(mFreeBindlessIndexQueue.PushBack(*(allocation.mHeapIndex)));

		allocation.mHeapIndex.reset();
	}

	void ResourceDescriptorHeap::CreateDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = IMPL::MAX_DESCRIPTORS_IN_SHADER_VISIBLE_HEAP;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;

		CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescriptorHeap)));
	}

	void ResourceDescriptorHeap::InitializeFreeBindlessIndexQueue()
	{
		for (std::uint32_t i = 0; i < IMPL::BINDLESS_SRV_SEGMENT_SIZE; ++i)
			assert(mFreeBindlessIndexQueue.PushBack(i));
	}

	std::atomic<std::uint32_t>& ResourceDescriptorHeap::GetCurrentPerFrameIndexTracker()
	{
		return mPerFrameIndexArr[Util::Engine::GetCurrentFrameNumber() % IMPL::PER_FRAME_SEGMENT_COUNT];
	}

	std::uint32_t ResourceDescriptorHeap::GetDescriptorHeapIndexFromPerFrameIndex(const std::uint32_t perFrameIndex) const
	{
		return (perFrameIndex + IMPL::BINDLESS_SRV_SEGMENT_SIZE + ((Util::Engine::GetCurrentFrameNumber() % IMPL::PER_FRAME_SEGMENT_COUNT) * IMPL::PER_FRAME_SEGMENT_SIZE));
	}

	void ResourceDescriptorHeap::InitializeDescriptorHandlesForAllocation(ResourceDescriptorHeapAllocation& allocation)
	{
		assert(allocation.mHeapIndex.has_value() && "ERROR: ResourceDescriptorHeap::InitializeDescriptorHandlesForAllocation() was called for a ResourceDescriptorHeapAllocation before it was given an index reservation!");
		
		allocation.mHandleInfo.CPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE{ mDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
		allocation.mHandleInfo.CPUHandle.Offset(*(allocation.mHeapIndex), mHandleIncrementSize);

		allocation.mHandleInfo.GPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE{ mDescriptorHeap->GetGPUDescriptorHandleForHeapStart() };
		allocation.mHandleInfo.GPUHandle.Offset(*(allocation.mHeapIndex), mHandleIncrementSize);
	}

	std::uint32_t ResourceDescriptorHeap::CreatePerFrameReservation(const std::uint32_t numDescriptors)
	{
		// Allocate the required per-frame descriptors.
		const std::uint32_t perFrameIndex = GetCurrentPerFrameIndexTracker().fetch_add(numDescriptors);

		// Ensure that we did not go over our budget.
		if (perFrameIndex + numDescriptors > IMPL::PER_FRAME_SEGMENT_SIZE) [[unlikely]]
			throw std::runtime_error{ "ERROR: The capacity for per-frame resource descriptors has been exceeded!" };

		return GetDescriptorHeapIndexFromPerFrameIndex(perFrameIndex);
	}
}