module;
#include <type_traits>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "DxDef.h"

module Brawler.D3DHeapPool;
import Brawler.ResourceCreationInfo;
import Brawler.CriticalSection;
import Util.General;
import Brawler.D3DHeap;
import Brawler.CriticalSection;
import Util.Math;
import Util.Engine;
import Brawler.D3DVideoBudgetInfo;

namespace
{
	static constexpr std::uint64_t MINIMUM_HEAP_SIZE = Util::Math::MegabytesToBytes(64);
}

namespace Brawler
{
	D3DHeapPool::HeapGroup::HeapGroup(HeapGroup&& rhs) noexcept :
		ResidentHeaps(),
		EvictedHeaps(),
		CritSection()
	{
		std::scoped_lock<CriticalSection> lock{ rhs.CritSection };

		ResidentHeaps = std::move(rhs.ResidentHeaps);
		EvictedHeaps = std::move(rhs.EvictedHeaps);
	}

	void D3DHeapPool::HeapGroup::MakeHeapResident(D3DHeap& heap)
	{
		for (auto itr = EvictedHeaps.begin(); itr != EvictedHeaps.end(); ++itr)
		{
			if (itr->get() == &heap)
			{
				heap.MakeResident();

				ResidentHeaps.push_back(std::move(*itr));
				EvictedHeaps.erase(itr);

				return;
			}
		}
	}

	void D3DHeapPool::HeapGroup::EvictHeap(D3DHeap& heap)
	{
		for (auto itr = ResidentHeaps.begin(); itr != ResidentHeaps.end(); ++itr)
		{
			if (itr->get() == &heap)
			{
				heap.Evict();

				EvictedHeaps.push_back(std::move(*itr));
				ResidentHeaps.erase(itr);

				return;
			}
		}
	}

	D3DHeapPool::D3DHeapPool(const Brawler::AllowedD3DResourceType resourceType) :
		mCritSection(),
		mResourceType(resourceType),
		mHeapGroupMap()
	{
		for (std::underlying_type_t<D3DHeapType> i = 0; i < Util::General::EnumCast(D3DHeapType::COUNT_OR_ERROR); ++i)
			mHeapGroupMap.emplace(std::pair<D3DHeapType, HeapGroup>(static_cast<D3DHeapType>(i), HeapGroup{}));
	}

	void D3DHeapPool::CreatePlacedResource(const Brawler::ResourceCreationInfo& creationInfo)
	{
		HeapGroup& relevantHeapGroup{ mHeapGroupMap.at(creationInfo.AccessMode) };
		std::scoped_lock<CriticalSection> lock{ relevantHeapGroup.CritSection };

		// First, try to create the allocation on a resident heap.
		for (auto& heapPtr : relevantHeapGroup.ResidentHeaps)
		{
			if (SUCCEEDED(heapPtr->AllocateResource(creationInfo)))
				return;
		}

		// If that failed, then try to find an evicted heap to store it in.
		for (auto& heapPtr : relevantHeapGroup.EvictedHeaps)
		{
			if (!heapPtr->WouldAllocationSucceed(creationInfo))
				continue;

			// HeapGroup::MakeHeapResident() will invalidate the iterator, so we
			// save the raw pointer before calling it.
			D3DHeap* suitableHeap = heapPtr.get();
			relevantHeapGroup.MakeHeapResident(*suitableHeap);

			HRESULT hr = suitableHeap->AllocateResource(creationInfo);
			assert(SUCCEEDED(hr));
			return;
		}

		// If that also failed, then we have no choice but to create a new
		// heap.
		std::unique_ptr<D3DHeap> createdHeap{ std::make_unique<D3DHeap>(*this, mResourceType) };
		D3D12_HEAP_DESC heapDesc{ Brawler::GetHeapDescriptionFromResourceCreationInfo(creationInfo) };
		heapDesc.SizeInBytes = std::max(heapDesc.SizeInBytes, MINIMUM_HEAP_SIZE);

		// If THIS fails, then we have run out of memory.
		//
		// (TODO: Can this be avoided with eviction to an extent?)
		CheckHRESULT(createdHeap->Initialize(heapDesc));

		HRESULT hr = createdHeap->AllocateResource(creationInfo);
		assert(SUCCEEDED(hr));

		relevantHeapGroup.ResidentHeaps.push_back(std::move(createdHeap));
	}
}