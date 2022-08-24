module;
#include <atomic>
#include <vector>
#include <queue>
#include <memory>
#include <span>
#include <optional>
#include <cassert>

module Brawler.VirtualTextureStreamer;
import Brawler.JobSystem;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureActivePageTracker;
import Brawler.GlobalTextureDatabase;

namespace Brawler
{
	VirtualTextureStreamer& VirtualTextureStreamer::GetInstance()
	{
		static VirtualTextureStreamer instance{};
		return instance;
	}

	void VirtualTextureStreamer::UpdateLogicalPage(const VirtualTextureLogicalPage logicalPage)
	{
		assert(logicalPage.VirtualTexturePtr != nullptr && "ERROR: An attempt was made to call VirtualTextureStreamer::UpdateLogicalPage() with a VirtualTextureLogicalPage instance whose VirtualTexturePtr field was nullptr!");
		mPendingRequestArr.PushBack(logicalPage);
	}

	void VirtualTextureStreamer::RequestGlobalTextureDefragmentation(const GlobalTextureDefragmentationType defragType)
	{
		assert(defragType != GlobalTextureDefragmentationType::NONE && "ERROR: It is pointless to call VirtualTextureStreamer::RequestGlobalTextureDefragmentation() while specifying a defragmentation type of GlobalTextureDefragmentationType::NONE!");
		
		GlobalTextureDefragmentationType prevDefragType{};

		do
		{
			prevDefragType = mRequestedDefragType.load(std::memory_order::relaxed);

			// Don't replace the previous value if it is "stronger" than what we are requesting.
			if (std::to_underlying(prevDefragType) >= std::to_underlying(defragType))
				return;
		} while (!mRequestedDefragType.compare_exchange_weak(prevDefragType, defragType, std::memory_order::relaxed));
	}

	void VirtualTextureStreamer::ExecuteStreamingPassAsync()
	{
		const bool wasEarlierPassRunning = mIsEarlierPassRunning.exchange(true, std::memory_order::acquire);

		// Only allow a single thread to be executing the virtual texture streaming pass
		// at a time. This is done because the virtual texture update system as a whole
		// is *NOT* thread safe. This was intentional: The thread-safe implementation of
		// global texture updates would require a significant amount of atomic operations at
		// best and a lot of locks at worst.
		//
		// This is fine, however, because virtual texture streaming can occur entirely
		// asynchronously with the rest of the engine. We wouldn't want to spend a lot of
		// CPU time doing this type of thing, anyways, so limiting it to a single thread
		// at a time allows other threads to work on more important things, such as SceneGraph
		// updates and D3D12 command list recording.

		if (wasEarlierPassRunning)
			return;

		Brawler::JobGroup streamingPassGroup{ Brawler::JobPriority::LOW };
		streamingPassGroup.AddJob([] ()
		{
			VirtualTextureStreamer::GetInstance().BeginStreamingPass();
		});

		// Execute the job asynchronously. That way, the calling thread (probably, but not 
		// necessarily, the main thread) can work on other tasks.
		streamingPassGroup.ExecuteJobsAsync();
	}

	void VirtualTextureStreamer::BeginStreamingPass()
	{
		/*
		Virtual Texture Streaming
		-------------------------

		Here is the general process as to how virtual texture streaming is executed:

		1. Generated requests are extracted from mPendingRequestArr. All of the extracted requests
		   will become part of the same update.

		2. The extracted requests are processed. What each request does varies depending on whether
		   or not the logical virtual texture page represented by the request is currently in GPU
		   memory.
		     - Requests for pages which are present in a GlobalTexture indicate that the page was
			   recently used. (This typically comes from the results of a GPU feedback pass.) The
			   GlobalTexture slot containing the page data then has its last used frame number
			   updated.

			-  Requests for pages which are missing in GPU memory indicate that the page must be
			   uploaded to a GlobalTexture. This request is marked within a GlobalTextureUpdateContext.

		3. VirtualTextureDatabase::TryCleanDeletedVirtualTextures() is called in order to actually
		   delete from CPU memory any VirtualTexture instances which were pending deletion. This
		   occurs as the result of a VirtualTextureHandle instance being deleted.

		4. If a defragmentation request was made, then it is fulfilled. We wait until after requests
		   have been processed to prevent a GlobalTexture from being deleted only to create a new
		   GlobalTexture immediately afterwards to fulfill any new page requests. In other words,
		   we do not want to delete a GlobalTexture instance if it could have been used to fulfill
		   a new page request. The previous GlobalTextureUpdateContext instances used in Step 2
		   is also used to perform defragmentation.

		5. If the GlobalTextureUpdateContext instance for this pass has any actual changes which
		   need to be made to the GlobalTextures on the GPU, then it is pushed to the back of
		   mPendingUpdateContextQueue. Otherwise, it is deleted by RAII when the function exits.

		6. mPendingUpdateContextQueue is iterated in a top-down manner to see if the
		   GlobalTextureUpdateContext instance at the front of the queue is ready to be submitted to
		   the GPUSceneUpdateRenderModule. If so, then the submission is made, and the next
		   GlobalTextureUpdateContext instance is checked. This process continues until either
		   mPendingUpdateContextQueue is empty or the GlobalTextureUpdateContext at the front of the
		   queue reports that it is not yet ready for submission.
		*/
		
		std::unique_ptr<GlobalTextureUpdateContext> updateContextPtr{ std::make_unique<GlobalTextureUpdateContext>() };
		std::vector<VirtualTextureLogicalPage> extractedRequestArr{};

		mPendingRequestArr.EraseIf([&extractedRequestArr] (VirtualTextureLogicalPage& logicalPage)
		{
			extractedRequestArr.push_back(std::move(logicalPage));
			return true;
		});

		HandleExtractedRequests(*updateContextPtr, std::span<const VirtualTextureLogicalPage>{ extractedRequestArr });

		// TODO: Finish this!
	}

	void VirtualTextureStreamer::HandleExtractedRequests(GlobalTextureUpdateContext& context, const std::span<const VirtualTextureLogicalPage> requestedPageSpan)
	{
		for (const auto& requestedPage : requestedPageSpan)
		{
			// Check to see if requestedPage already exists in a GlobalTexture. If so, then we simply
			// need to update the frame number for its slot. Otherwise, we need to add the page data
			// to a GlobalTexture.
			assert(requestedPage.VirtualTexturePtr != nullptr);

			const VirtualTextureActivePageTracker::TrackedPage currTrackedPage{
				.LogicalMipLevel = requestedPage.LogicalMipLevel,
				.LogicalPageXCoordinate = requestedPage.LogicalPageXCoordinate,
				.LogicalPageYCoordinate = requestedPage.LogicalPageYCoordinate
			};
			const std::optional<GlobalTexturePageIdentifier> storagePageIdentifier{ requestedPage.VirtualTexturePtr->GetActivePageTracker().GetStoragePageIdentifier(currTrackedPage) };

			if (storagePageIdentifier.has_value())
				GlobalTextureDatabase::GetInstance().NotifyGlobalTextureForUseInCurrentFrame(*storagePageIdentifier);
			else
				GlobalTextureDatabase::GetInstance().AddVirtualTexturePage(context, requestedPage);
		}
	}
}