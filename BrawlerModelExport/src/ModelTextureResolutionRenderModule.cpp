module;
#include <vector>
#include <functional>
#include <mutex>

module Brawler.ModelTextureResolutionRenderModule;
import Brawler.JobSystem;

namespace Brawler
{
	ModelTextureResolutionEventHandle ModelTextureResolutionRenderModule::RegisterTextureResolutionCallback(std::move_only_function<void(D3D12::FrameGraphBuilder&)>&& callback)
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		mCallbackArr.push_back(std::move(callback));

		return ModelTextureResolutionEventHandle{};
	}

	void ModelTextureResolutionRenderModule::BuildFrameGraph(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<std::move_only_function<void(D3D12::FrameGraphBuilder&)>> currCallbackArr{};

		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			currCallbackArr = std::move(mCallbackArr);
		}

		std::vector<D3D12::FrameGraphBuilder> subBuilderArr{};
		subBuilderArr.reserve(currCallbackArr.size());

		Brawler::JobGroup createSubBuildersGroup{};
		createSubBuildersGroup.Reserve(currCallbackArr.size());

		std::size_t currIndex = 0;
		for (auto& callback : currCallbackArr)
		{
			subBuilderArr.push_back(D3D12::FrameGraphBuilder{ builder.GetFrameGraph() });

			createSubBuildersGroup.AddJob([currIndex, &callback, &subBuilderArr] ()
			{
				D3D12::FrameGraphBuilder& currSubBuilder{ subBuilderArr[currIndex] };
				callback(currSubBuilder);
			});

			++currIndex;
		}

		createSubBuildersGroup.ExecuteJobs();

		for (auto&& subBuilder : subBuilderArr)
			builder.MergeFrameGraphBuilder(std::move(subBuilder));
	}
}