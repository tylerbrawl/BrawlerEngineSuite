module;
#include <vector>
#include <atomic>
#include <ranges>
#include <span>

module Brawler.D3D12.GPUCommandContextSink;
import Util.Engine;
import Brawler.D3D12.GPUCommandManager;

namespace Brawler
{
	namespace D3D12
	{
		std::span<GPUCommandContextSubmissionPoint> GPUCommandContextSink::InitializeSinkForCurrentFrame(const std::size_t numExecutionModules)
		{
			mSinkNotifier.store(0);
			mSubmitPointArr = std::vector<GPUCommandContextSubmissionPoint>{ numExecutionModules };

			for (auto& submitPoint : mSubmitPointArr)
				submitPoint.SetSinkNotifier(mSinkNotifier);

			return std::span<GPUCommandContextSubmissionPoint>{ mSubmitPointArr };
		}

		void GPUCommandContextSink::RunGPUSubmissionLoop()
		{
			std::size_t numSubmitPointsHandled = 0;
			const auto submitCmdContextGroupsLambda = [] (const std::span<GPUCommandContextGroup> cmdContextGroupSpan)
			{
				for (auto& cmdContextGroup : cmdContextGroupSpan)
					Util::Engine::GetGPUCommandManager().SubmitGPUCommandContextGroup(std::move(cmdContextGroup));
			};

			while (numSubmitPointsHandled < mSubmitPointArr.size())
			{
				// Store the current value of mSinkNotifier here.
				const std::uint64_t oldNotifierValue = mSinkNotifier.load();

				// Try to submit the GPUCommandContextGroups of each GPUExecutionModule in
				// order.
				const std::size_t oldNumSubmitPointsHandled = numSubmitPointsHandled;
				for (auto& submitPoint : mSubmitPointArr | std::views::drop(oldNumSubmitPointsHandled))
				{
					std::vector<GPUCommandContextGroup> cmdContextGroupArr{ submitPoint.ExtractGPUCommandContextGroups() };

					if (!cmdContextGroupArr.empty())
					{
						submitCmdContextGroupsLambda(std::span<GPUCommandContextGroup>{cmdContextGroupArr});
						++numSubmitPointsHandled;
					}
					else
						break;
				}

				// If we still have more GPUCommandContextGroups to submit from other
				// GPUCommandContextSubmissionPoints, then we wait here.
				if (numSubmitPointsHandled < mSubmitPointArr.size())
					mSinkNotifier.wait(oldNotifierValue);
			}
		}
	}
}