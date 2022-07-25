module;
#include <memory>
#include <vector>
#include <span>
#include <cassert>

module Brawler.VirtualTextureManagementSubModule;
import Brawler.JobSystem;
import Util.Engine;

namespace Brawler
{
	void VirtualTextureManagementSubModule::CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUploadBuffer>&& preparedBufferPtr)
	{
		mPreparedBufferPtrArr.PushBack(std::move(preparedBufferPtr));
	}

	bool VirtualTextureManagementSubModule::HasCommittedGlobalTextureChanges() const
	{
		// Don't bother if we do not have any buffers ready to handle.
		return !mPreparedBufferPtrArr.Empty();
	}

	void VirtualTextureManagementSubModule::PrepareForGlobalTextureUpdates()
	{
		// Before we begin recording commands, we need to select the buffers which we are going
		// to be working with for this frame. We should also delete any buffers which are no longer
		// needed.
		//
		// This is safe because the function is never called concurrently, since FrameGraph building
		// for the next frame cannot start until FrameGraph building for the previous frame has
		// finished.
		
		// Move all of the std::unique_ptr<GlobalTextureUploadBuffer> instances into the std::vector
		// instance containing the buffers which we will use for this frame. We use 
		// Brawler::ThreadSafeVector::EraseIf() to make this an atomic operation.
		mPreparedBufferPtrArr.EraseIf([this] (std::unique_ptr<GlobalTextureUploadBuffer>&& bufferPtr)
		{
			mCurrentBufferArr.push_back(std::move(bufferPtr));
			return true;
		});

		// Delete any GlobalTextureUploadBuffer instances which are no longer needed.
		const std::uint64_t currentFrameNumber = Util::Engine::GetCurrentFrameNumber();
		std::erase_if(mBuffersPendingDeletionArr, [currentFrameNumber] (const ManagedUploadBuffer& managedBuffer)
		{
			return (currentFrameNumber >= managedBuffer.SafeDeletionFrameNumber);
		});
	}

	void VirtualTextureManagementSubModule::FinishGlobalTextureUpdates()
	{
		// Move all of the GlobalTextureUploadBuffer instances which we just used into
		// mBuffersPendingDeletionArr.
		for (auto&& bufferPtr : mCurrentBufferArr)
		{
			mBuffersPendingDeletionArr.push_back(ManagedUploadBuffer{
				.ManagedBufferPtr{ std::move(bufferPtr) },
				.SafeDeletionFrameNumber = (Util::Engine::GetCurrentFrameNumber() + Util::Engine::MAX_FRAMES_IN_FLIGHT)
			});
		}

		mCurrentBufferArr.clear();
	}

	auto VirtualTextureManagementSubModule::CreateIndirectionTextureUpdatesRenderPass(D3D12::FrameGraphBuilder& builder) const
	{
		/*
		Updating Indirection Textures
		-------------------------------

		For each GlobalTexturePageSwapOperation within the current array of GlobalTextureUploadBuffer
		instances, we need to at the minimum update the indirection texture of the page which is being
		written into the global texture. We might also need to update the indirection texture of a
		page which is being replaced, if this is applicable.

		In order to support the combined page, indirection textures in the Brawler Engine are somewhat
		different from indirection textures described in the literature. For every mip level not contained
		within the combined page, each 2x2 quad within its corresponding indirection texture mip level
		represents one page. This is different from the one texel per page which is commonly used. This
		was done so that the final mip level of the indirection texture can describe the page coordinates
		of the combined page.

		Contrary to GPU scene buffer updates and global texture updates, indirection texture updates
		are done on the DIRECT queue. This is primarily done because indirection textures are created
		as render targets, and we might need to transition the resource from D3D12_RESOURCE_STATE_RENDER_TARGET
		to D3D12_RESOURCE_STATE_COPY_XXX; this transition can only be done on the DIRECT queue. An added
		benefit of this is that on AMD hardware, according to https://gpuopen.com/performance/, work on
		the DIRECT queue can proceed uninterrupted concurrently with work on the COPY queue, so long as
		the tasks are independent (which they are in this particular case).
		*/

		assert(!mCurrentBufferArr.empty());

		IndirectionTextureUpdater updater{};

		for (const auto& uploadBufferPtr : mCurrentBufferArr)
		{
			const std::span<const std::unique_ptr<GlobalTexturePageSwapOperation>> pageSwapOperationSpan{ uploadBufferPtr->GetPageSwapOperationSpan() };
			assert(!pageSwapOperationSpan.empty());

			for (const auto& pageSwapOperationPtr : pageSwapOperationSpan)
				updater.AddUpdatesForPageSwapOperation(*pageSwapOperationPtr);
		}

		return updater.CreateIndirectionTextureUpdatesRenderPass(builder);
	}
}