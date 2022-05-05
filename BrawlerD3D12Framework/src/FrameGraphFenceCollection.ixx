module;
#include <array>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraphFenceCollection;
import Brawler.D3D12.GPUFence;
import Brawler.Win32.SafeHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphFenceCollection
		{
		public:
			enum class FenceIndex
			{
				DIRECT_QUEUE,
				COMPUTE_QUEUE,
				COPY_QUEUE,

				COUNT_OR_ERROR
			};

		private:
			struct FenceInfo
			{
				GPUFence Fence;
				Brawler::Win32::SafeHandle HEvent;
			};

		public:
			FrameGraphFenceCollection() = default;

			FrameGraphFenceCollection(const FrameGraphFenceCollection& rhs) = delete;
			FrameGraphFenceCollection& operator=(const FrameGraphFenceCollection& rhs) = delete;

			FrameGraphFenceCollection(FrameGraphFenceCollection&& rhs) noexcept = default;
			FrameGraphFenceCollection& operator=(FrameGraphFenceCollection&& rhs) noexcept = default;

			void Initialize();
			void Reset();

			template <FrameGraphFenceCollection::FenceIndex Index>
				requires (Index != FrameGraphFenceCollection::FenceIndex::COUNT_OR_ERROR)
			void SignalFence(Brawler::D3D12CommandQueue& cmdQueue);

			void WaitForFrameGraphCompletion() const;

			void SetMakeResidentFence(GPUFence&& residencyFence);
			void EnsureGPUResidencyForCommandQueue(Brawler::D3D12CommandQueue& cmdQueue) const;

		private:
			std::array<FenceInfo, std::to_underlying(FenceIndex::COUNT_OR_ERROR)> mFenceInfoArr;
			std::optional<GPUFence> mMakeResidentFence;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <FrameGraphFenceCollection::FenceIndex Index>
			requires (Index != FrameGraphFenceCollection::FenceIndex::COUNT_OR_ERROR)
		void FrameGraphFenceCollection::SignalFence(Brawler::D3D12CommandQueue& cmdQueue)
		{
			FenceInfo& fenceInfo{ mFenceInfoArr[std::to_underlying(Index)] };
			
			fenceInfo.Fence.SignalOnGPUTimeline(cmdQueue);
			fenceInfo.Fence.SignalEventOnCompletion(fenceInfo.HEvent.get());
		}
	}
}