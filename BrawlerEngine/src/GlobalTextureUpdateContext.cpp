module;
#include <unordered_map>
#include <optional>
#include <vector>
#include <memory>
#include <cassert>

module Brawler.GlobalTextureUpdateContext;
import Brawler.VirtualTexturePageStates;

namespace Brawler
{
	void GlobalTextureUpdateContext::OnPageAddedToGlobalTexture(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& pageInfo)
	{
		if (!mPageStateMap.contains(logicalPage))
			mPageStateMap[logicalPage] = VirtualTexturePageNeutralState{};

		// I Can't Believe This Builds (TM)!
		std::optional<PolymorphicAdapter<I_VirtualTexturePageState>> optionalNewStateAdapter{};
		mPageStateMap[logicalPage].AccessData([&optionalNewStateAdapter, &pageInfo]<typename StateType>(StateType & state)
		{
			auto optionalNewState{ state.OnPageAddedToGlobalTexture(std::move(pageInfo)) };

			if (optionalNewState.has_value()) [[likely]]
				optionalNewStateAdapter = std::move(*optionalNewState);
		});

		if (optionalNewStateAdapter.has_value()) [[likely]]
			mPageStateMap[logicalPage] = std::move(*optionalNewStateAdapter);
	}

	void GlobalTextureUpdateContext::OnPageRemovedFromGlobalTexture(const VirtualTextureLogicalPage& logicalPage, GlobalTexturePageInfo&& pageInfo)
	{
		if (!mPageStateMap.contains(logicalPage))
			mPageStateMap[logicalPage] = VirtualTexturePageNeutralState{};

		std::optional<PolymorphicAdapter<I_VirtualTexturePageState>> optionalNewStateAdapter{};
		mPageStateMap[logicalPage].AccessData([&pageInfo, &optionalNewStateAdapter]<typename StateType>(StateType & state)
		{
			auto optionalNewState{ state.OnPageRemovedFromGlobalTexture(std::move(pageInfo)) };

			if (optionalNewState.has_value()) [[likely]]
				optionalNewStateAdapter = std::move(*optionalNewState);
		});

		if (optionalNewStateAdapter.has_value()) [[likely]]
			mPageStateMap[logicalPage] = std::move(*optionalNewStateAdapter);
	}

	void GlobalTextureUpdateContext::FinalizeContext()
	{
		FinalizeVirtualTexturePageStates();
		CreatePageRequests();
	}

	bool GlobalTextureUpdateContext::HasPendingGlobalTextureChanges() const
	{
		return (mPageUploadSet.has_value() || !mTransferRequestPtrArr.empty() || !mRemovalRequestPtrArr.empty());
	}

	bool GlobalTextureUpdateContext::ReadyForGPUSubmission() const
	{
		if (!HasPendingGlobalTextureChanges())
			return true;

		// We only need to wait for page data used in upload requests to be uploaded into the
		// upload buffer. If no upload requests exists, then we can immediately proceed with GPU
		// submission.
		return (!mPageUploadSet.has_value() || mPageUploadSet->ReadyForGlobalTextureUploads());
	}

	void GlobalTextureUpdateContext::FinalizeVirtualTexturePageStates()
	{
		for (auto& [logicalPage, stateAdapter] : mPageStateMap)
		{
			std::optional<PolymorphicAdapter<I_VirtualTexturePageState>> optionalFinalStateAdapter{};
			stateAdapter.AccessData([&optionalFinalStateAdapter]<typename StateType>(StateType& state)
			{
				auto optionalFinalState{ state.OnContextFinalization() };

				if (optionalFinalState.has_value())
					optionalFinalStateAdapter = std::move(*optionalFinalState);
			});

			if (optionalFinalStateAdapter.has_value())
				stateAdapter = std::move(*optionalFinalStateAdapter);
		}
	}

	void GlobalTextureUpdateContext::CreatePageRequests()
	{
		for (auto& [logicalPage, stateAdapter] : mPageStateMap)
		{
			stateAdapter.AccessData([this, &logicalPage]<typename StateType>(StateType & state)
			{
				auto operationDetailsResult{ state.GetOperationDetails(logicalPage) };
				using ResultType = std::decay_t<decltype(operationDetailsResult)>;

				if constexpr (std::is_same_v<ResultType, std::unique_ptr<GlobalTexturePageUploadRequest>>)
				{
					if (!mPageUploadSet.has_value())
						mPageUploadSet = GlobalTexturePageUploadSet{};

					mPageUploadSet->AddPageUploadRequest(std::move(operationDetailsResult));
				}

				else if constexpr (std::is_same_v<ResultType, std::unique_ptr<GlobalTexturePageTransferRequest>>)
					mTransferRequestPtrArr.push_back(std::move(operationDetailsResult));

				else if constexpr (std::is_same_v<ResultType, std::unique_ptr<GlobalTexturePageRemovalRequest>>)
					mRemovalRequestPtrArr.push_back(std::move(operationDetailsResult));
			});
		}

		if (mPageUploadSet.has_value())
		{
			assert(mPageUploadSet->HasActiveUploadRequests());
			mPageUploadSet->PrepareRequestedPageData();
		}
	}
}