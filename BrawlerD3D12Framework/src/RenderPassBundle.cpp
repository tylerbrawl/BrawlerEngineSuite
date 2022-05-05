module;
#include <vector>

module Brawler.D3D12.RenderPassBundle;
import Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceLifetimeType;

namespace Brawler
{
	namespace D3D12
	{
		std::size_t RenderPassBundle::GetTotalRenderPassCount() const
		{
			return (mDirectPassArr.size() + mComputePassArr.size() + mCopyPassArr.size());
		}

		Brawler::CompositeEnum<GPUCommandQueueType> RenderPassBundle::GetUsedQueues() const
		{
			Brawler::CompositeEnum<GPUCommandQueueType> usedQueues{};

			if (!mDirectPassArr.empty())
				usedQueues |= GPUCommandQueueType::DIRECT;

			if (!mComputePassArr.empty())
				usedQueues |= GPUCommandQueueType::COMPUTE;

			if (!mCopyPassArr.empty())
				usedQueues |= GPUCommandQueueType::COPY;

			return usedQueues;
		}

		std::span<I_GPUResource* const> RenderPassBundle::GetResourceDependencies() const
		{
			return mResourceDependencyArr.CreateSpan();
		}

		void RenderPassBundle::UpdateTransientGPUResourceAliasTracker(TransientGPUResourceAliasTracker& aliasTracker) const
		{
			// We want to let the TransientGPUResourceAliasTracker know about all of the transient GPU
			// resources which are used by the RenderPasses contained within this RenderPassBundle.

			mResourceDependencyArr.ForEach([this, &aliasTracker] (I_GPUResource* const& resourcePtr)
			{
				if (resourcePtr->GetGPUResourceLifetimeType() == GPUResourceLifetimeType::TRANSIENT)
					aliasTracker.AddTransientResourceDependencyForBundle(mBundleID, *resourcePtr);
			});
		}

		void RenderPassBundle::MarkAsSyncPoint()
		{
			mIsSyncPoint = true;
		}

		bool RenderPassBundle::IsSyncPoint() const
		{
			return mIsSyncPoint;
		}

		void RenderPassBundle::SetRenderPassBundleID(const std::uint32_t bundleID)
		{
			mBundleID = bundleID;
		}

		std::uint32_t RenderPassBundle::GetRenderPassBundleID() const
		{
			return mBundleID;
		}
	}
}