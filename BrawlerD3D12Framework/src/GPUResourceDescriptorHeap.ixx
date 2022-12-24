module;
#include <cstddef>
#include <queue>
#include <mutex>
#include <cassert>
#include <memory>
#include <array>
#include <atomic>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.DescriptorHandleInfo;
import Brawler.D3D12.BindlessSRVSentinel;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		constexpr std::size_t RESOURCE_DESCRIPTOR_HEAP_SIZE = 1000000;
		constexpr std::size_t BINDLESS_SRVS_PARTITION_SIZE = 500000;

		/// <summary>
		/// This is the size of the partition of the GPUResourceDescriptorHeap which is
		/// dedicated to storing per-frame descriptor tables. Each frame "owns" half of
		/// this partition for storing descriptor tables.
		/// </summary>
		constexpr std::size_t PER_FRAME_DESCRIPTORS_PARTITION_SIZE = (RESOURCE_DESCRIPTOR_HEAP_SIZE - BINDLESS_SRVS_PARTITION_SIZE);
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap
		{
		private:
			struct BindlessIndexInfo
			{
				// There's 500,000 bindless SRV indices. If we use a Brawler::ThreadSafeQueue, then
				// we get 500,000 unsigned 32-bit integers allocated on the stack. I'm embarrassed to
				// say that I only realized how bad of an idea this is after I got a stack overflow.
				std::queue<std::uint32_t> Queue;

				mutable std::mutex CritSection;
			};

		public:
			GPUResourceDescriptorHeap() = default;

			GPUResourceDescriptorHeap(const GPUResourceDescriptorHeap& rhs) = delete;
			GPUResourceDescriptorHeap& operator=(const GPUResourceDescriptorHeap& rhs) = delete;

			GPUResourceDescriptorHeap(GPUResourceDescriptorHeap&& rhs) noexcept = default;
			GPUResourceDescriptorHeap& operator=(GPUResourceDescriptorHeap&& rhs) noexcept = default;

			void InitializeBindlessSRVQueue();
			void InitializeD3D12DescriptorHeap();

			std::unique_ptr<BindlessSRVSentinel> AllocateBindlessSRV();
			void ReClaimBindlessSRV(BindlessSRVSentinel& srvAllocation);

			DescriptorHandleInfo CreatePerFrameDescriptorHeapReservation(const std::uint32_t numDescriptors);

			void ResetPerFrameDescriptorHeapIndex();

			Brawler::D3D12DescriptorHeap& GetD3D12DescriptorHeap() const;

			__forceinline CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;
			__forceinline CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			__forceinline std::uint32_t GetBasePerFrameDescriptorHeapIndex() const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mHeap;
			BindlessIndexInfo mBindlessIndexQueue;

			// Another weird bug was found! Using std::atomic<std::uint32_t> instead
			// of std::atomic<std::uint64_t> now triggers an internal compiler error
			// (ICE) in the MSVC due to a regression on their end.
			std::array<std::atomic<std::uint64_t>, Util::Engine::MAX_FRAMES_IN_FLIGHT> mPerFrameIndexArr;

			std::uint32_t mDescriptorHandleIncrementSize;
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
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), mDescriptorHandleIncrementSize };
		}

		__forceinline CD3DX12_GPU_DESCRIPTOR_HANDLE GPUResourceDescriptorHeap::GetGPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_GPU_DESCRIPTOR_HANDLE{ mHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), mDescriptorHandleIncrementSize };
		}
	}
}