module;
#include <vector>
#include <atomic>
#include <span>

export module Brawler.D3D12.GPUCommandContextSink;
import Brawler.D3D12.GPUCommandContextSubmissionPoint;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextSink
		{
		public:
			GPUCommandContextSink() = default;

			GPUCommandContextSink(const GPUCommandContextSink& rhs) = delete;
			GPUCommandContextSink& operator=(const GPUCommandContextSink& rhs) = delete;

			GPUCommandContextSink(GPUCommandContextSink&& rhs) noexcept = default;
			GPUCommandContextSink& operator=(GPUCommandContextSink&& rhs) noexcept = default;

			std::span<GPUCommandContextSubmissionPoint> InitializeSinkForCurrentFrame(const std::size_t numExecutionModules);

			/// <summary>
			/// When this function is called, the thread which calls it will loop until
			/// all of the GPUCommandContexts for this frame have been sent to the
			/// GPUCommandManager.
			/// 
			/// NOTE: Util::Coroutine::TryExecuteJob() is *NOT* called within this function.
			/// The reason is that if the thread within this function happens to pick up a
			/// long-running job, then the GPU could potentially be left starved of work to
			/// execute.
			/// 
			/// Instead, the thread will use a std::atomic wait to let the OS notify it of
			/// when useful work can be done. (This is more performant and less CPU hungry
			/// than a busy wait.)
			/// </summary>
			void RunGPUSubmissionLoop();

		private:
			std::vector<GPUCommandContextSubmissionPoint> mSubmitPointArr;
			std::atomic<std::uint64_t> mSinkNotifier;
		};
	}
}