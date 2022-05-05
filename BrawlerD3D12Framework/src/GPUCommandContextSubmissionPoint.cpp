module;
#include <atomic>
#include <vector>
#include <mutex>
#include <cassert>

module Brawler.D3D12.GPUCommandContextSubmissionPoint;

namespace Brawler
{
	namespace D3D12
	{
		void GPUCommandContextSubmissionPoint::SubmitGPUCommandContextGroups(std::vector<GPUCommandContextGroup>&& contextGroupArr)
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			assert(mCmdContextGroupArr.empty() && "ERROR: An attempt was made to submit GPUCommandContextGroups to a GPUCommandContextSubmissionPoint, but its previous commands were not submitted to the GPU!");

			mCmdContextGroupArr = std::move(contextGroupArr);

			// Notify the owning GPUCommandContextSink that a GPUExecutionModule has submitted its
			// GPUCommandContexts for the frame.
			assert(mSinkNotifierPtr != nullptr && "ERROR: A GPUCommandContextSubmissionPoint was never assigned a GPUCommandContextSink notifier pointer!");
			mSinkNotifierPtr->fetch_add(1);
			mSinkNotifierPtr->notify_all();
		}

		std::vector<GPUCommandContextGroup> GPUCommandContextSubmissionPoint::ExtractGPUCommandContextGroups()
		{
			std::vector<GPUCommandContextGroup> extractedGroupArr{};

			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				extractedGroupArr = std::move(mCmdContextGroupArr);
			}

			return extractedGroupArr;
		}

		void GPUCommandContextSubmissionPoint::SetSinkNotifier(std::atomic<std::uint64_t>& sinkNotifier)
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };
			mSinkNotifierPtr = &sinkNotifier;
		}
	}
}