module;
#include <cstdint>
#include <cassert>
#include <optional>
#include <ranges>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceRTVDSVHeap;
import Util.Engine;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		template <NonShaderVisibleDescriptorType DescriptorType>
		GPUResourceRTVDSVHeapAllocation<DescriptorType>::GPUResourceRTVDSVHeapAllocation(const std::uint32_t heapIndex) :
			mAllocatedIndex(heapIndex)
		{}
		
		template <NonShaderVisibleDescriptorType DescriptorType>
		GPUResourceRTVDSVHeapAllocation<DescriptorType>::~GPUResourceRTVDSVHeapAllocation()
		{
			ReturnAllocation();
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		GPUResourceRTVDSVHeapAllocation<DescriptorType>::GPUResourceRTVDSVHeapAllocation(GPUResourceRTVDSVHeapAllocation&& rhs) noexcept :
			mAllocatedIndex(std::move(rhs.mAllocatedIndex))
		{
			rhs.mAllocatedIndex.reset();
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		GPUResourceRTVDSVHeapAllocation<DescriptorType>& GPUResourceRTVDSVHeapAllocation<DescriptorType>::operator=(GPUResourceRTVDSVHeapAllocation&& rhs) noexcept
		{
			ReturnAllocation();

			mAllocatedIndex = std::move(rhs.mAllocatedIndex);
			rhs.mAllocatedIndex.reset();

			return *this;
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		CD3DX12_CPU_DESCRIPTOR_HANDLE GPUResourceRTVDSVHeapAllocation<DescriptorType>::GetAssignedDescriptorHandle() const
		{
			assert(mAllocatedIndex.has_value());
			return GPUResourceRTVDSVHeap<DescriptorType>::GetInstance().GetCPUDescriptorHandle(*mAllocatedIndex);
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		void GPUResourceRTVDSVHeapAllocation<DescriptorType>::ReturnAllocation()
		{
			if (mAllocatedIndex.has_value()) [[likely]]
			{
				GPUResourceRTVDSVHeap<DescriptorType>::GetInstance().ReturnReservation(*this);

				// The GPUResourceRTVDSVHeap is responsible for nullifying this GPUResourceRTVDSVHeapAllocation's
				// index.
				assert(!mAllocatedIndex.has_value());
			}
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <NonShaderVisibleDescriptorType DescriptorType>
		GPUResourceRTVDSVHeap<DescriptorType>& GPUResourceRTVDSVHeap<DescriptorType>::GetInstance()
		{
			static GPUResourceRTVDSVHeap<DescriptorType> instance{};
			return instance;
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		void GPUResourceRTVDSVHeap<DescriptorType>::InitializeAvailableDescriptorIndexQueue()
		{
			for (const auto i : std::views::iota(0u, NSVDescriptorHeapInfo<DescriptorType>::NUM_DESCRIPTORS))
			{
				const bool pushResult = mAvailableIndexQueue.PushBack(i);
				assert(pushResult);
			}
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		void GPUResourceRTVDSVHeap<DescriptorType>::InitializeD3D12DescriptorHeap()
		{
			static constexpr D3D12_DESCRIPTOR_HEAP_DESC HEAP_DESC{
				.Type = NSVDescriptorHeapInfo<DescriptorType>::DESCRIPTOR_HEAP_TYPE,
				.NumDescriptors = NSVDescriptorHeapInfo<DescriptorType>::NUM_DESCRIPTORS,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};
			
			assert(mDescriptorHeapPtr == nullptr && "ERROR: An attempt was made to re-create the ID3D12DescriptorHeap of a GPUResourceRTVDSVHeap!");
			Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&HEAP_DESC, IID_PPV_ARGS(&mDescriptorHeapPtr)));
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		GPUResourceRTVDSVHeapAllocation<DescriptorType> GPUResourceRTVDSVHeap<DescriptorType>::CreateReservation()
		{
			std::optional<std::uint32_t> extractedIndex{ mAvailableIndexQueue.TryPop() };

			// Use an if-constexpr statement to differentiate between the two asserts. This helps
			// us to identify which heap ran out of space.
			if constexpr (DescriptorType == NonShaderVisibleDescriptorType::RTV)
				assert(extractedIndex.has_value() && "ERROR: The limit of RTVs within the GPUResourceRTVHeap has been exceeded! (See NSVDescriptorHeapInfo in GPUResourceRTVDSVHeap.ixx.)");
			else
				assert(extractedIndex.has_value() && "ERROR: The limit of DSVs within the GPUResourceDSVHeap has been exceeded! (See NSVDescriptorHeapInfo in GPUResourceRTVDSVHeap.ixx.)");

			return GPUResourceRTVDSVHeapAllocation<DescriptorType>{ *extractedIndex };
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		void GPUResourceRTVDSVHeap<DescriptorType>::ReturnReservation(GPUResourceRTVDSVHeapAllocation<DescriptorType>& allocation)
		{
			assert(allocation.mAllocatedIndex.has_value());
			const bool pushResult = mAvailableIndexQueue.PushBack(*(allocation.mAllocatedIndex));

			assert(pushResult && "ERROR: Somehow, too many values were added to the index queue of a GPUResourceRTVDSVHeap!");

			allocation.mAllocatedIndex.reset();
		}

		template <NonShaderVisibleDescriptorType DescriptorType>
		CD3DX12_CPU_DESCRIPTOR_HANDLE GPUResourceRTVDSVHeap<DescriptorType>::GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			assert(mDescriptorHeapPtr != nullptr);
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mDescriptorHeapPtr->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(NSVDescriptorHeapInfo<DescriptorType>::DESCRIPTOR_HEAP_TYPE) };
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template class GPUResourceRTVDSVHeapAllocation<NonShaderVisibleDescriptorType::RTV>;
		template class GPUResourceRTVDSVHeapAllocation<NonShaderVisibleDescriptorType::DSV>;

		template class GPUResourceRTVDSVHeap<NonShaderVisibleDescriptorType::RTV>;
		template class GPUResourceRTVDSVHeap<NonShaderVisibleDescriptorType::DSV>;
	}
}