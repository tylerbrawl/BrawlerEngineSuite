module;
#include <array>
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.FrameGraphFenceCollection;

namespace Brawler
{
	namespace D3D12
	{
		void FrameGraphFenceCollection::Initialize()
		{
			for (auto& fenceInfo : mFenceInfoArr)
			{
				fenceInfo.Fence.Initialize();

				fenceInfo.HEvent = Brawler::Win32::SafeHandle{ CreateEvent(nullptr, FALSE, TRUE, nullptr) };
				assert(fenceInfo.HEvent.get() != nullptr);
			}
		}

		void FrameGraphFenceCollection::Reset()
		{
			mMakeResidentFence.reset();
		}

		void FrameGraphFenceCollection::WaitForFrameGraphCompletion() const
		{
			// There is actually a race condition with using GPUFence::WaitOnCPUTimeline() on
			// the GPUFence instances owned by this FrameGraphFenceCollection instance. Specifically,
			// the function may be called before the thread which submits commands to the GPU ever
			// has a chance to signal the fences on the GPU timeline in the first place. Thus, when
			// the thread wanting to reset the FrameGraph goes to wait on the fences, it will immediately
			// return because their signal value was never set.
			
			for (const auto& fenceInfo : mFenceInfoArr)
				WaitForSingleObject(fenceInfo.HEvent.get(), INFINITE);
		}

		void FrameGraphFenceCollection::SetMakeResidentFence(GPUFence&& residencyFence)
		{
			mMakeResidentFence = std::move(residencyFence);
		}

		void FrameGraphFenceCollection::EnsureGPUResidencyForCommandQueue(Brawler::D3D12CommandQueue& cmdQueue) const
		{
			if (mMakeResidentFence.has_value()) [[unlikely]]
				mMakeResidentFence->WaitOnGPUTimeline(cmdQueue);
		}
	}
}