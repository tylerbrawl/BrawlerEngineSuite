module;
#include <cstddef>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceDescriptors.GPUResourceDescriptorHeap;
import Brawler.D3D12.GPUResourceDescriptors.BindlessSRVAllocation;
import Brawler.D3D12.GPUResourceDescriptors.PerFrameDescriptorTable;
import Brawler.D3D12.GPUResourceDescriptors.DescriptorHandleInfo;
import Brawler.ThreadSafeQueue;
import Util.Engine;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
		class DescriptorTableBuilder;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t RESOURCE_DESCRIPTOR_HEAP_SIZE = 1000000;
		static constexpr std::size_t BINDLESS_SRVS_PARTITION_SIZE = 500000;

		/// <summary>
		/// This is the size of the partition of the GPUResourceDescriptorHeap which is
		/// dedicated to storing per-frame descriptor tables. Each frame "owns" half of
		/// this partition for storing descriptor tables.
		/// </summary>
		static constexpr std::size_t PER_FRAME_DESCRIPTORS_PARTITION_SIZE = (RESOURCE_DESCRIPTOR_HEAP_SIZE - BINDLESS_SRVS_PARTITION_SIZE);
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap
		{
		public:
			GPUResourceDescriptorHeap() = default;

			GPUResourceDescriptorHeap(const GPUResourceDescriptorHeap& rhs) = delete;
			GPUResourceDescriptorHeap& operator=(const GPUResourceDescriptorHeap& rhs) = delete;

			GPUResourceDescriptorHeap(GPUResourceDescriptorHeap&& rhs) noexcept = default;
			GPUResourceDescriptorHeap& operator=(GPUResourceDescriptorHeap&& rhs) noexcept = default;

			void Initialize();

			/// <summary>
			/// Creates a bindless SRV for the specified I_GPUResource. The returned BindlessSRVAllocation
			/// represents the location in the descriptor heap at which the descriptor was created.
			/// </summary>
			/// <param name="resource">
			/// - The GPU resource for which a bindless SRV is to be created.
			/// </param>
			/// <returns>
			/// The function returns a BindlessSrvAllocation which represents the location in the
			/// descriptor heap at which the descriptor was created.
			/// </returns>
			BindlessSRVAllocation CreateBindlessSRV(I_GPUResource& resource);

			void ReClaimBindlessSRV(BindlessSRVAllocation& srvAllocation);

			PerFrameDescriptorTable CreatePerFrameDescriptorTable(const DescriptorTableBuilder& tableBuilder);

			void ResetPerFrameDescriptorHeapIndex();

			__forceinline CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;
			__forceinline CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			__forceinline std::uint32_t GetBasePerFrameDescriptorHeapIndex() const;

			DescriptorHandleInfo CreatePerFrameDescriptorHeapReservation(const std::uint32_t numDescriptors);

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mHeap;
			Brawler::ThreadSafeQueue<std::uint32_t, BINDLESS_SRVS_PARTITION_SIZE> mBindlessIndexQueue;
			std::atomic<std::uint32_t> mPerFrameIndex;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		__forceinline CD3DX12_CPU_DESCRIPTOR_HANDLE GPUResourceDescriptorHeap::GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}

		__forceinline CD3DX12_GPU_DESCRIPTOR_HANDLE GPUResourceDescriptorHeap::GetGPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_GPU_DESCRIPTOR_HANDLE{ mHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}
	}
}