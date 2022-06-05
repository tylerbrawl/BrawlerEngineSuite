module;
#include <vector>
#include <span>

module Brawler.D3D12.GPUResourceEventCollection;

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceEventCollection::MergeGPUResourceEventCollection(GPUResourceEventCollection&& mergedCollection)
		{
			mDirectEventArr.reserve(mDirectEventArr.size() + mergedCollection.mDirectEventArr.size());
			mComputeEventArr.reserve(mComputeEventArr.size() + mergedCollection.mComputeEventArr.size());
			mCopyEventArr.reserve(mCopyEventArr.size() + mergedCollection.mCopyEventArr.size());

			for (auto&& directEvent : mergedCollection.mDirectEventArr)
				mDirectEventArr.push_back(std::move(directEvent));

			for (auto&& computeEvent : mergedCollection.mComputeEventArr)
				mComputeEventArr.push_back(std::move(computeEvent));

			for (auto&& copyEvent : mergedCollection.mCopyEventArr)
				mCopyEventArr.push_back(std::move(copyEvent));
		}
	}
}