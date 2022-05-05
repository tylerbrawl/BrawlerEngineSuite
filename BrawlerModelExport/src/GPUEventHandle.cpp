module;
#include <vector>
#include "DxDef.h"

module Brawler.D3D12.GPUEventHandle;
import Util.Coroutine;

namespace Brawler
{
	namespace D3D12
	{
		bool GPUEventHandle::HasGPUFinishedExecution() const
		{
			for (const auto& fenceInfo : mFenceInfoArr)
			{
				if (fenceInfo.Fence->GetCompletedValue() < fenceInfo.RequiredFenceValue)
					return false;
			}

			return true;
		}

		void GPUEventHandle::WaitForGPUExecution() const
		{
			while (!HasGPUFinishedExecution())
				Util::Coroutine::TryExecuteJob();
		}

		void GPUEventHandle::AddFence(Brawler::D3D12Fence& fence, const std::uint64_t requiredFenceValue)
		{
			mFenceInfoArr.push_back(FenceInfo{
				.Fence{ &fence },
				.RequiredFenceValue = requiredFenceValue
			});
		}
	}
}