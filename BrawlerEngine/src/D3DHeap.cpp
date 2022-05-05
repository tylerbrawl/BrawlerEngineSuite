module;
#include <cstdint>
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3DHeap;
import Brawler.DisplayAdapter;
import Brawler.ResourceCreationInfo;
import Util.Math;
import Brawler.D3DHeapBuddyAllocator;
import Brawler.D3DHeapPool;
import Brawler.D3DHeapAllocationHandle;
import Brawler.I_GPUResource;
import Util.Engine;

namespace
{
	// This represents the number of frames for which a D3DHeap's usage history is
	// tracked for. Setting this number too high means that rarely used resources
	// will remain in memory, even if they do not need to be. Setting this number
	// too low means that frequently used D3D12Heaps may be evicted frequently,
	// leading to large amounts of stuttering.
	//
	// TODO: Find a suitable value for this through experimentation. Theoretically,
	// it seems like this number should be made relatively large. After all, in my
	// opinion, stuttering is MUCH worse than wasted memory.
	static constexpr std::uint32_t MAX_HISTORY_QUEUE_SIZE = 30;

	bool IsHeapDescriptionCompliant(const D3D12_HEAP_DESC& heapDesc)
	{
		if ((heapDesc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS) == D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS)
			return !(heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS);

		if ((heapDesc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES) == D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES)
			return !(heapDesc.Flags & D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);

		if ((heapDesc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES) == D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES)
			return !(heapDesc.Flags & D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES);

		return false;
	}
}

namespace Brawler
{
	D3DHeap::RelevanceHistory::RelevanceHistory() :
		HistoryQueue(),
		HistoryRelevanceCounter(0)
	{}

	void D3DHeap::RelevanceHistory::UpdateHistory(const std::uint32_t newRelevanceCount)
	{
		// If the history queue is too large, then remove the oldest counter from
		// the total count.
		if (IsHistoryFullyPrepared())
		{
			HistoryRelevanceCounter -= HistoryQueue.front();
			HistoryQueue.pop();
		}
		
		HistoryRelevanceCounter += newRelevanceCount;
		HistoryQueue.push(newRelevanceCount);
	}

	std::uint32_t D3DHeap::RelevanceHistory::GetHistoryCount() const
	{
		return HistoryRelevanceCounter;
	}

	bool D3DHeap::RelevanceHistory::IsHistoryFullyPrepared() const
	{
		return (HistoryQueue.size() == MAX_HISTORY_QUEUE_SIZE);
	}

	D3DHeap::D3DHeap(D3DHeapPool& owningPool, const Brawler::AllowedD3DResourceType allowedType) :
		mRelevanceCounter(0),
		mHistory(),
		mAllocator(),
		mHeap(nullptr),
		mType(D3DHeapType::COUNT_OR_ERROR),
		mOwningPool(&owningPool),
		mState(D3DHeapState::UNINITIALIZED),
		mHeapInfo(GetHeapInfoFromAllowedResourceType(allowedType))
	{}

	HRESULT D3DHeap::Initialize(const D3D12_HEAP_DESC& heapDesc)
	{
		// Do not allow a re-initialization of a heap.
		assert(mHeap == nullptr && "ERROR: An attempt was made to call D3DHeap::Initialize() more than once for a given D3DHeap instance!");
		
		// Ensure that we are creating a heap which can be used on Heap Tier 1 devices.
		assert(IsHeapDescriptionCompliant(heapDesc) && "ERROR: An attempt was made to create a D3DHeap whose ID3D12Heap instance would NOT be compatible with Heap Tier 1 devices!");

		// Ensure that the heap size is a power-of-two value.
		assert(Util::Math::CountOneBits(heapDesc.SizeInBytes) == 1 && "ERROR: An attempt was made to create a D3DHeap with a non-power-of-two size!");

		Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };
		HRESULT hr = d3dDevice.CreateHeap1(
			&heapDesc,
			nullptr,
			IID_PPV_ARGS(&mHeap)
		);

		if (SUCCEEDED(hr))
		{
			mType = Brawler::GetHeapTypeFromHeapDescription(heapDesc);

			if (heapDesc.Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT)
				mState = D3DHeapState::EVICTED;
			else
				mState = D3DHeapState::RESIDENT;

			mAllocator.Initialize(*this);
		}

		return hr;
	}

	HRESULT D3DHeap::AllocateResource(const Brawler::ResourceCreationInfo& creationInfo)
	{
		std::optional<D3DHeapAllocationHandle> hAllocation{ mAllocator.Allocate(creationInfo.AllocationInfo) };

		if (hAllocation)
		{
			Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };
			Microsoft::WRL::ComPtr<Brawler::D3D12Resource> d3dResource{};

			HRESULT hr = d3dDevice.CreatePlacedResource1(
				mHeap.Get(),
				hAllocation->Offset,
				&(creationInfo.ResourceDesc),
				creationInfo.InitialState,
				(creationInfo.OptimizedClearValue ? &(*(creationInfo.OptimizedClearValue)) : nullptr),
				IID_PPV_ARGS(&d3dResource)
			);

			if (FAILED(hr))
			{
				HRESULT deviceRemovedReason = Util::Engine::GetD3D12Device().GetDeviceRemovedReason();
				assert(false && "ERROR: ID3D12Device8::CreatePlacedResource1() failed, even though it probably shouldn't have!");
			}

			// If the allocation was successful, then update the I_GPUResource.
			hAllocation->OwningHeap = this;
			creationInfo.Resource->InitializeIMPL(std::move(d3dResource), std::move(*hAllocation), creationInfo.InitialState);

			return S_OK;
		}

		return E_OUTOFMEMORY;
	}

	bool D3DHeap::WouldAllocationSucceed(const Brawler::ResourceCreationInfo& creationInfo)
	{
		return mAllocator.Allocate(creationInfo.AllocationInfo).has_value();
	}

	HRESULT D3DHeap::MakeResident()
	{
		HRESULT hr = Util::Engine::GetD3D12Device().MakeResident(1, reinterpret_cast<ID3D12Pageable**>(mHeap.GetAddressOf()));

		if (SUCCEEDED(hr))
			mState = D3DHeapState::RESIDENT;

		return hr;
	}

	HRESULT D3DHeap::Evict()
	{
		// TODO: Before we can actually evict the heap, we need to make sure that the resources within
		// it are not in use by the GPU. The idea, then, is to use a fence to signal the completion
		// of every call to ExecuteCommandLists() during a frame. Then, when it comes time to evict
		// the heap, we wait for the signal representing the last call to ExecuteCommandLists() which
		// involved a given resource.
		
		assert(false && "ERROR: We need to implement a way to check for resource usage before evicting an ID3D12Heap! (See the comments in the definition of D3DHeap::Evict()!)");

		HRESULT hr = Util::Engine::GetD3D12Device().Evict(1, reinterpret_cast<ID3D12Pageable**>(mHeap.GetAddressOf()));

		if (SUCCEEDED(hr))
			mState = D3DHeapState::EVICTED;

		return hr;
	}

	void D3DHeap::PrepareForNextFrame()
	{
		// Add the previous frame's relevance counter to the history and
		// reset the counter for the next frame.
		mHistory.UpdateHistory(mRelevanceCounter.load());
		mRelevanceCounter.store(0);
	}

	void D3DHeap::IncrementCurrentRelevanceCount()
	{
		mRelevanceCounter.fetch_add(1);
	}

	D3DHeapType D3DHeap::GetHeapType() const
	{
		return mType;
	}

	const D3DHeapInfo& D3DHeap::GetHeapInfo() const
	{
		return mHeapInfo;
	}

	std::uint64_t D3DHeap::GetHeapSize() const
	{
		assert(mHeap != nullptr && "ERROR: An attempt was made to get the size of an uninitialized D3DHeap!");

		return mHeap->GetDesc().SizeInBytes;
	}
}