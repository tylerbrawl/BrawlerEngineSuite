module;
#include <atomic>

module Brawler.VirtualTextureStreamer;
import Brawler.JobSystem;

namespace Brawler
{
	VirtualTextureStreamer& VirtualTextureStreamer::GetInstance()
	{
		static VirtualTextureStreamer instance{};
		return instance;
	}

	void VirtualTextureStreamer::UpdateLogicalPage(const VirtualTextureLogicalPage logicalPage)
	{
		mPendingRequestArr.PushBack(logicalPage);
	}

	void VirtualTextureStreamer::ExecuteStreamingPassAsync()
	{
		const bool wasEarlierPassRunning = mIsEarlierPassRunning.exchange(true, std::memory_order::relaxed);

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
}