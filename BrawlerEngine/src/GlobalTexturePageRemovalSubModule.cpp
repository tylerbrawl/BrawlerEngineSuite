module;
#include <vector>
#include <memory>
#include <span>

module Brawler.GlobalTexturePageRemovalSubModule;
import Brawler.D3D12.FrameGraphBuilding;

namespace Brawler
{
	void GlobalTexturePageRemovalSubModule::AddPageRemovalRequests(const std::span<const std::unique_ptr<GlobalTexturePageRemovalRequest>> removalRequestSpan)
	{
		for (auto&& requestPtr : removalRequestSpan)
			mRequestPtrArr.push_back(std::move(requestPtr));
	}

	bool GlobalTexturePageRemovalSubModule::HasPageRemovalRequests() const
	{
		return !mRequestPtrArr.empty();
	}

	std::vector<GlobalTexturePageRemovalSubModule::IndirectionTextureUpdateRenderPass_T> GlobalTexturePageRemovalSubModule::GetPageRemovalRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		IndirectionTextureUpdater updater{};

		for (const auto& removalRequestPtr : mRequestPtrArr)
			updater.ClearIndirectionTextureEntry(removalRequestPtr->GetLogicalPage());

		mRequestPtrArr.clear();

		return updater.FinalizeIndirectionTextureUpdates(builder);
	}
}