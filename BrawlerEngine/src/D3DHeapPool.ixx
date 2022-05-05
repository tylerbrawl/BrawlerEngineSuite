module;
#include <unordered_map>
#include <memory>
#include <vector>
#include "DxDef.h"

export module Brawler.D3DHeapPool;
import Brawler.ResourceCreationInfo;
import Brawler.CriticalSection;
import Brawler.D3DHeap;

export namespace Brawler
{
	class D3DHeapPool
	{
	private:
		struct HeapGroup
		{
			std::vector<std::unique_ptr<D3DHeap>> ResidentHeaps;
			std::vector<std::unique_ptr<D3DHeap>> EvictedHeaps;
			CriticalSection CritSection;

			HeapGroup() = default;

			HeapGroup(const HeapGroup& rhs) = delete;
			HeapGroup& operator=(const HeapGroup& rhs) = delete;

			HeapGroup(HeapGroup&& rhs) noexcept;
			HeapGroup& operator=(HeapGroup&& rhs) noexcept = delete;

			void MakeHeapResident(D3DHeap& heap);
			void EvictHeap(D3DHeap& head);
		};

	public:
		explicit D3DHeapPool(const Brawler::AllowedD3DResourceType resourceType);

		D3DHeapPool(const D3DHeapPool& rhs) = delete;
		D3DHeapPool& operator=(const D3DHeapPool& rhs) = delete;

		D3DHeapPool(D3DHeapPool&& rhs) noexcept = default;
		D3DHeapPool& operator=(D3DHeapPool&& rhs) noexcept = default;

		void CreatePlacedResource(const Brawler::ResourceCreationInfo& creationInfo);

	private:
		CriticalSection mCritSection;
		const Brawler::AllowedD3DResourceType mResourceType;
		std::unordered_map<Brawler::D3DHeapType, HeapGroup> mHeapGroupMap;
	};
}