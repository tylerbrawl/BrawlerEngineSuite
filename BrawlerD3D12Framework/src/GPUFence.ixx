module;
#include "DxDef.h"

export module Brawler.D3D12.GPUFence;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUFence
		{
		public:
			GPUFence() = default;

			GPUFence(const GPUFence& rhs) = delete;
			GPUFence& operator=(const GPUFence& rhs) = delete;

			GPUFence(GPUFence&& rhs) noexcept = default;
			GPUFence& operator=(GPUFence&& rhs) noexcept = default;

			void Initialize();
			void Initialize(Microsoft::WRL::ComPtr<Brawler::D3D12Fence>&& d3dFence, const std::uint64_t currSignalValue);

			void SignalOnCPUTimeline();
			void SignalOnGPUTimeline(Brawler::D3D12CommandQueue& cmdQueue);

			void WaitOnCPUTimeline() const;
			void WaitOnGPUTimeline(Brawler::D3D12CommandQueue& cmdQueue) const;

			std::uint64_t GetLastSignalValue() const;
			bool HasFenceCompletedValue(const std::uint64_t fenceValue) const;

			void SignalEventOnCompletion(const HANDLE hEvent) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12Fence> mFence;
			std::uint64_t mCurrSignalValue;
		};
	}
}