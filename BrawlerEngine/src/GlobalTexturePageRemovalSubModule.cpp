module;
#include <vector>
#include <memory>
#include <span>
#include <mutex>

module Brawler.GlobalTexturePageRemovalSubModule;
import Brawler.D3D12.FrameGraphBuilding;

namespace Brawler
{
	void GlobalTexturePageRemovalSubModule::AddPageRemovalRequests(const std::span<const std::unique_ptr<GlobalTexturePageRemovalRequest>> removalRequestSpan)
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };

		for (auto&& requestPtr : removalRequestSpan)
			mRequestPtrArr.push_back(std::move(requestPtr));
	}

	bool GlobalTexturePageRemovalSubModule::HasPageRemovalRequests() const
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };
		return !mRequestPtrArr.empty();
	}

	std::vector<GlobalTexturePageRemovalSubModule::IndirectionTextureUpdateRenderPass_T> GlobalTexturePageRemovalSubModule::GetPageRemovalRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<std::unique_ptr<GlobalTexturePageRemovalRequest>> currRemovalRequestPtrArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };
			currRemovalRequestPtrArr = std::move(mRequestPtrArr);
		}

		IndirectionTextureUpdater updater{};

		for (const auto& removalRequestPtr : currRemovalRequestPtrArr)
			updater.ClearIndirectionTextureEntry(removalRequestPtr->GetLogicalPage());

		return updater.FinalizeIndirectionTextureUpdates(builder);
	}
}