module;
#include <atomic>
#include <vector>
#include <mutex>

export module Brawler.D3D12.GPUCommandContextSubmissionPoint;
import Brawler.D3D12.GPUCommandContextGroup;

namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextSink;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextSubmissionPoint
		{
		private:
			friend class GPUCommandContextSink;

		public:
			GPUCommandContextSubmissionPoint() = default;

			GPUCommandContextSubmissionPoint(const GPUCommandContextSubmissionPoint& rhs) = delete;
			GPUCommandContextSubmissionPoint& operator=(const GPUCommandContextSubmissionPoint& rhs) = delete;

			GPUCommandContextSubmissionPoint(GPUCommandContextSubmissionPoint&& rhs) noexcept = default;
			GPUCommandContextSubmissionPoint& operator=(GPUCommandContextSubmissionPoint&& rhs) noexcept = default;

			void SubmitGPUCommandContextGroups(std::vector<GPUCommandContextGroup>&& contextGroupSpan);
			std::vector<GPUCommandContextGroup> ExtractGPUCommandContextGroups();

		private:
			void SetSinkNotifier(std::atomic<std::uint64_t>& sinkNotifier);

		private:
			std::vector<GPUCommandContextGroup> mCmdContextGroupArr;
			std::atomic<std::uint64_t>* mSinkNotifierPtr;
			mutable std::mutex mCritSection;
		};
	}
}