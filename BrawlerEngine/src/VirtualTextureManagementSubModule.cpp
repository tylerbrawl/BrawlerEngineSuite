module;
#include <memory>
#include <vector>
#include <span>
#include <cassert>
#include <mutex>
#include <optional>
#include <span>

module Brawler.VirtualTextureManagementSubModule;
import Brawler.JobSystem;
import Util.Engine;
import Brawler.GlobalTexturePageUploadSet;

namespace Brawler
{
	void VirtualTextureManagementSubModule::CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUpdateContext>&& updateContextPtr)
	{
		assert(updateContextPtr->ReadyForGPUSubmission() && "ERROR: A GlobalTextureUpdateContext instance was submitted to the VirtualTextureManagementSubModule before its changes were ready to be applied on the GPU!");

		const std::scoped_lock<std::mutex> lock{ mCritSection };
		mPendingContextPtrArr.push_back(std::move(updateContextPtr));
	}

	bool VirtualTextureManagementSubModule::HasCommittedGlobalTextureChanges() const
	{
		// Don't bother if we do not have any buffers ready to handle.
		const std::scoped_lock<std::mutex> lock{ mCritSection };
		return (mPageUploadSubModule.HasPageUploadRequests() || mPageRemovalSubModule.HasPageRemovalRequests() || mPageTransferSubModule.HasPageTransferRequests());
	}

	VirtualTextureManagementPassCollection VirtualTextureManagementSubModule::CreateGlobalTextureChangeRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<std::unique_ptr<GlobalTextureUpdateContext>> extractedUploadContextPtrArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };
			extractedUploadContextPtrArr = std::move(mPendingContextPtrArr);
		}

		if (extractedUploadContextPtrArr.empty())
			return VirtualTextureManagementPassCollection{};

		// Add the GlobalTexture update passes based on the GlobalTextureUpdateContext which contains them. That
		// way, updates are applied in the correct order.
		for (const auto& uploadContextPtr : extractedUploadContextPtrArr)
		{
			std::optional<GlobalTexturePageUploadSet> currUploadSet{ uploadContextPtr->ExtractPageUploadSet() };

			if (currUploadSet.has_value())
				mPageUploadSubModule.AddPageUploadRequests(std::move(*currUploadSet));

			std::vector<std::unique_ptr<GlobalTexturePageRemovalRequest>> currRemovalRequestPtrArr{ uploadContextPtr->ExtractPageRemovalRequests() };
			mPageRemovalSubModule.AddPageRemovalRequests(std::span<std::unique_ptr<GlobalTexturePageRemovalRequest>>{ currRemovalRequestPtrArr });

			std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> currTransferRequestPtrArr{ uploadContextPtr->ExtractPageTransferRequests() };
			mPageTransferSubModule.AddPageTransferRequests(std::span<std::unique_ptr<GlobalTexturePageTransferRequest>>{ currTransferRequestPtrArr });
		}

		Brawler::JobGroup globalTextureChangePassGroup{};
		globalTextureChangePassGroup.Reserve(3);

		GlobalTexturePageUploadSubModule::GlobalTextureUploadPassCollection uploadPassCollection{};
		D3D12::FrameGraphBuilder uploadPassBuilder{ builder.GetFrameGraph() };

		globalTextureChangePassGroup.AddJob([this, &uploadPassCollection, &uploadPassBuilder] ()
		{
			uploadPassCollection = mPageUploadSubModule.GetPageUploadRenderPasses(uploadPassBuilder);
		});

		std::vector<GlobalTexturePageRemovalSubModule::IndirectionTextureUpdateRenderPass_T> removalPassArr{};
		D3D12::FrameGraphBuilder removalPassBuilder{ builder.GetFrameGraph() };

		globalTextureChangePassGroup.AddJob([this, &removalPassArr, &removalPassBuilder] ()
		{
			removalPassArr = mPageRemovalSubModule.GetPageRemovalRenderPasses(removalPassBuilder);
		});

		GlobalTexturePageTransferSubModule::GlobalTextureTransferPassCollection transferPassCollection{};
		D3D12::FrameGraphBuilder transferPassBuilder{ builder.GetFrameGraph() };

		globalTextureChangePassGroup.AddJob([this, &transferPassCollection, &transferPassBuilder] ()
		{
			transferPassCollection = mPageTransferSubModule.GetPageTransferRenderPasses(transferPassBuilder);
		});

		globalTextureChangePassGroup.ExecuteJobs();

		VirtualTextureManagementPassCollection globalTextureChangePassCollection{};
		globalTextureChangePassCollection.SetGlobalTextureUploadPasses(std::move(uploadPassCollection));
		globalTextureChangePassCollection.SetGlobalTextureRemovalPasses(std::move(removalPassArr));
		globalTextureChangePassCollection.SetGlobalTextureTransferPasses(std::move(transferPassCollection));

		return globalTextureChangePassCollection;
	}
}