module;
#include <vector>
#include <mutex>
#include <span>
#include <memory>
#include <cassert>

module Brawler.GlobalTexturePageUploadSubModule;
import Brawler.JobSystem;
import Brawler.GlobalTexturePageUploadRequest;
import Brawler.D3D12.Texture2D;
import Brawler.GlobalTexturePageInfo;
import Util.Engine;

namespace Brawler
{
	void GlobalTexturePageUploadSubModule::AddPageUploadRequests(GlobalTexturePageUploadSet&& uploadSet)
	{
		assert(uploadSet.HasActiveUploadRequests() && "ERROR: An attempt was made to provide a GlobalTexturePageUploadSubModule with a GlobalTexturePageUploadSet which did not have any actual requests!");
		assert(uploadSet.ReadyForGlobalTextureUploads() && "ERROR: An attempt was made to provide a GlobalTexturePageUploadSubModule with a GlobalTexturePageUploadSet whose request data has not finished streaming!");

		const std::scoped_lock<std::mutex> lock{ mCritSection };
		mUploadSetArr.push_back(std::move(uploadSet));
	}

	bool GlobalTexturePageUploadSubModule::HasPageUploadRequests() const
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };
		return !mUploadSetArr.empty();
	}

	GlobalTexturePageUploadSubModule::GlobalTextureUploadPassCollection GlobalTexturePageUploadSubModule::GetPageUploadRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		CheckForUploadBufferDeletions();
		
		std::vector<GlobalTexturePageUploadSet> extractedUploadSetArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };
			extractedUploadSetArr = std::move(mUploadSetArr);
		}

		if (extractedUploadSetArr.empty())
			return GlobalTextureUploadPassCollection{};

		const std::span<const GlobalTexturePageUploadSet> extractedUploadSetSpan{ extractedUploadSetArr };

		Brawler::JobGroup pageUploadGroup{};
		pageUploadGroup.Reserve(2);

		std::vector<GlobalTextureCopyRenderPass_T> globalTextureCopyPassArr{};
		pageUploadGroup.AddJob([&globalTextureCopyPassArr, extractedUploadSetSpan] ()
		{
			globalTextureCopyPassArr = CreateGlobalTextureCopyRenderPasses(extractedUploadSetSpan);
		});

		std::vector<IndirectionTextureUpdateRenderPass_T> indirectionTextureUpdatePassArr{};
		pageUploadGroup.AddJob([&indirectionTextureUpdatePassArr, &builder, extractedUploadSetSpan] ()
		{
			indirectionTextureUpdatePassArr = CreateIndirectionTextureUpdateRenderPasses(builder, extractedUploadSetSpan);
		});

		pageUploadGroup.ExecuteJobs();

		// The GlobalTexturePageUploadSet instances own the upload BufferResource which contains the page
		// data which must be copied to the GlobalTextures; if we destroy the GlobalTexturePageUploadSet
		// instances, then we will also destroy these buffers. We do not want to do that until we know that
		// the GPU has finished accessing the relevant information.
		//
		// To account for this, we move the BufferResource instances representing the upload buffers into an 
		// array of PendingUploadBufferDeletion instances for later deletion. We do not need any synchronization 
		// on this array because the Brawler Engine only does FrameGraph building one frame at a time; thus, this 
		// function will never be called concurrently by multiple threads.
		const std::uint64_t safeDeletionFrameNumber = (Util::Engine::GetCurrentFrameNumber() + Util::Engine::MAX_FRAMES_IN_FLIGHT);

		for (auto& extractedUploadSet : extractedUploadSetArr)
			mPendingDeletionArr.emplace_back(std::move(extractedUploadSet.ExtractUploadBufferResource()), safeDeletionFrameNumber);

		return GlobalTextureUploadPassCollection{
			.GlobalTextureCopyPassArr{ std::move(globalTextureCopyPassArr) },
			.IndirectionTextureUpdatePassArr{ std::move(indirectionTextureUpdatePassArr) }
		};
	}

	void GlobalTexturePageUploadSubModule::CheckForUploadBufferDeletions()
	{
		std::erase_if(mPendingDeletionArr, [] (const PendingUploadBufferDeletion& pendingDeletion) { return (Util::Engine::GetCurrentFrameNumber() >= pendingDeletion.SafeDeletionFrameNumber); });
	}

	std::vector<GlobalTexturePageUploadSubModule::GlobalTextureCopyRenderPass_T> GlobalTexturePageUploadSubModule::CreateGlobalTextureCopyRenderPasses(const std::span<const GlobalTexturePageUploadSet> uploadSetSpan)
	{
		std::vector<GlobalTextureCopyRenderPass_T> globalTextureCopyPassArr{};
		globalTextureCopyPassArr.reserve(uploadSetSpan.size());

		for (const auto& uploadSet : uploadSetSpan)
		{
			const std::span<const std::unique_ptr<GlobalTexturePageUploadRequest>> uploadRequestSpan{ uploadSet.GetUploadRequestSpan() };

			for (const auto& uploadRequestPtr : uploadRequestSpan)
			{
				GlobalTextureCopyRenderPass_T currCopyPass{};
				currCopyPass.SetRenderPassName("Virtual Texture Management: GlobalTexture Page Uploads - GlobalTexture Page Update Pass");

				D3D12::Texture2DSubResource destinationSubResource{ uploadRequestPtr->GetDestinationGlobalTexturePageInfo().GetGlobalTexture2D().GetSubResource(0) };
				currCopyPass.AddResourceDependency(destinationSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
				currCopyPass.AddResourceDependency(uploadRequestPtr->GetPageDataBufferSubAllocation(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

				currCopyPass.SetInputData(GlobalTextureCopyPassInfo{
					.DestCopyRegion{ uploadRequestPtr->GetDestinationGlobalTexturePageInfo().GetPageCopyRegion() },
					.SrcCopySnapshot{ uploadRequestPtr->GetPageDataBufferSubAllocation() }
				});

				currCopyPass.SetRenderPassCommands([] (const GlobalTextureCopyPassInfo& passInfo)
				{
					context.CopyBufferToTexture(passInfo.DestCopyRegion, passInfo.SrcCopySnapshot);
				});

				globalTextureCopyPassArr.push_back(std::move(currCopyPass));
			}
		}

		return globalTextureCopyPassArr;
	}

	std::vector<GlobalTexturePageUploadSubModule::IndirectionTextureUpdateRenderPass_T> GlobalTexturePageUploadSubModule::CreateIndirectionTextureUpdateRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const GlobalTexturePageUploadSet> uploadSetSpan)
	{
		IndirectionTextureUpdater updater{};

		for (const auto& uploadSet : uploadSetSpan)
		{
			const std::span<const std::unique_ptr<GlobalTexturePageUploadRequest>> uploadRequestSpan{ uploadSet.GetUploadRequestSpan() };

			for (const auto& uploadRequestPtr : uploadRequestSpan)
				updater.UpdateIndirectionTextureEntry(uploadRequestPtr->GetLogicalPage(), uploadRequestPtr->GetDestinationGlobalTexturePageInfo());
		}

		return updater.FinalizeIndirectionTextureUpdates(builder);
	}
}