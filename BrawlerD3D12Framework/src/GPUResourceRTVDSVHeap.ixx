module;
#include <cstdint>
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceRTVDSVHeap;
import Brawler.D3D12.NonShaderVisibleDescriptorType;
import Brawler.ThreadSafeQueue;

namespace Brawler
{
	namespace D3D12
	{
		template <NonShaderVisibleDescriptorType DescriptorType>
		struct NSVDescriptorHeapInfo
		{
			static_assert(sizeof(DescriptorType) != sizeof(DescriptorType));
		};

		template <std::size_t NumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
		struct NSVDescriptorHeapInfoInstantiation
		{
			static constexpr std::size_t NUM_DESCRIPTORS = NumDescriptors;
			static constexpr D3D12_DESCRIPTOR_HEAP_TYPE DESCRIPTOR_HEAP_TYPE = HeapType;
		};

		template <>
		struct NSVDescriptorHeapInfo<NonShaderVisibleDescriptorType::RTV> : public NSVDescriptorHeapInfoInstantiation<5000, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV>
		{};

		template <>
		struct NSVDescriptorHeapInfo<NonShaderVisibleDescriptorType::DSV> : public NSVDescriptorHeapInfoInstantiation<5000, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV>
		{};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <NonShaderVisibleDescriptorType DescriptorType>
		class GPUResourceRTVDSVHeap;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <NonShaderVisibleDescriptorType DescriptorType>
		class GPUResourceRTVDSVHeapAllocation
		{
		private:
			friend class GPUResourceRTVDSVHeap<DescriptorType>;

		public:
			GPUResourceRTVDSVHeapAllocation() = default;

		private:
			explicit GPUResourceRTVDSVHeapAllocation(const std::uint32_t heapIndex);

		public:
			~GPUResourceRTVDSVHeapAllocation();

			GPUResourceRTVDSVHeapAllocation(const GPUResourceRTVDSVHeapAllocation& rhs) = delete;
			GPUResourceRTVDSVHeapAllocation& operator=(const GPUResourceRTVDSVHeapAllocation& rhs) = delete;

			GPUResourceRTVDSVHeapAllocation(GPUResourceRTVDSVHeapAllocation&& rhs) noexcept;
			GPUResourceRTVDSVHeapAllocation& operator=(GPUResourceRTVDSVHeapAllocation&& rhs) noexcept;

			CD3DX12_CPU_DESCRIPTOR_HANDLE GetAssignedDescriptorHandle() const;

		private:
			void ReturnAllocation();

		private:
			std::optional<std::uint32_t> mAllocatedIndex;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <NonShaderVisibleDescriptorType DescriptorType>
		class GPUResourceRTVDSVHeap
		{
		private:
			GPUResourceRTVDSVHeap() = default;

		public:
			~GPUResourceRTVDSVHeap() = default;

			GPUResourceRTVDSVHeap(const GPUResourceRTVDSVHeap& rhs) = delete;
			GPUResourceRTVDSVHeap& operator=(const GPUResourceRTVDSVHeap& rhs) = delete;

			GPUResourceRTVDSVHeap(GPUResourceRTVDSVHeap&& rhs) noexcept = delete;
			GPUResourceRTVDSVHeap& operator=(GPUResourceRTVDSVHeap&& rhs) noexcept = delete;

			static GPUResourceRTVDSVHeap& GetInstance();

			void InitializeAvailableDescriptorIndexQueue();
			void InitializeD3D12DescriptorHeap();

			GPUResourceRTVDSVHeapAllocation<DescriptorType> CreateReservation();
			void ReturnReservation(GPUResourceRTVDSVHeapAllocation<DescriptorType>& allocation);

			CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mDescriptorHeapPtr;
			Brawler::ThreadSafeQueue<std::uint32_t, NSVDescriptorHeapInfo<DescriptorType>::NUM_DESCRIPTORS> mAvailableIndexQueue;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		using GPUResourceRTVHeapAllocation = GPUResourceRTVDSVHeapAllocation<NonShaderVisibleDescriptorType::RTV>;
		using GPUResourceDSVHeapAllocation = GPUResourceRTVDSVHeapAllocation<NonShaderVisibleDescriptorType::DSV>;

		using GPUResourceRTVHeap = GPUResourceRTVDSVHeap<NonShaderVisibleDescriptorType::RTV>;
		using GPUResourceDSVHeap = GPUResourceRTVDSVHeap<NonShaderVisibleDescriptorType::DSV>;
	}
}