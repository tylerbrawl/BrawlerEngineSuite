module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUFence;
import Util.Engine;
import Util.Win32;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		void GPUFence::Initialize()
		{
			Microsoft::WRL::ComPtr<ID3D12Fence> oldVersionFence{};
			Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateFence(
				mCurrSignalValue,
				D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(&oldVersionFence)
			));

			Util::General::CheckHRESULT(oldVersionFence.As(&mFence));
		}

		void GPUFence::Initialize(Microsoft::WRL::ComPtr<Brawler::D3D12Fence>&& d3dFence, const std::uint64_t signalValue)
		{
			mFence = std::move(d3dFence);
			mCurrSignalValue = signalValue;
		}

		void GPUFence::SignalOnCPUTimeline()
		{
			Util::General::CheckHRESULT(mFence->Signal(++mCurrSignalValue));
		}

		void GPUFence::SignalOnGPUTimeline(Brawler::D3D12CommandQueue& cmdQueue)
		{
			cmdQueue.Signal(mFence.Get(), ++mCurrSignalValue);
		}

		void GPUFence::WaitOnCPUTimeline() const
		{
			// The MSDN states that by specifying nullptr for the hEvent parameter of
			// ID3D12Fence::SetEventOnCompletion(), the API will not return until the fence
			// reaches the specified fence value.
			//
			// This is indeed the desired behavior, but one can only hope that this is
			// implemented internally with a synchronization primitive, rather than a busy
			// wait.

			Util::General::CheckHRESULT(mFence->SetEventOnCompletion(mCurrSignalValue, nullptr));
		}

		void GPUFence::WaitOnGPUTimeline(Brawler::D3D12CommandQueue& cmdQueue) const
		{
			Util::General::CheckHRESULT(cmdQueue.Wait(mFence.Get(), mCurrSignalValue));
		}

		std::uint64_t GPUFence::GetLastSignalValue() const
		{
			return mCurrSignalValue;
		}

		bool GPUFence::HasFenceCompletedValue(const std::uint64_t fenceValue) const
		{
			return (mFence->GetCompletedValue() >= fenceValue);
		}

		void GPUFence::SignalEventOnCompletion(const HANDLE hEvent) const
		{
			assert(Util::Win32::IsHandleValid(hEvent));

			Util::General::CheckHRESULT(mFence->SetEventOnCompletion(mCurrSignalValue, hEvent));
		}
	}
}