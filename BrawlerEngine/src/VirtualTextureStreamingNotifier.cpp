module;
#include <cassert>

module Brawler.VirtualTextureStreamingNotifier;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Util.Engine;

namespace Brawler
{
	VirtualTextureStreamingNotifier::VirtualTextureStreamingNotifier(const VirtualTextureLogicalPage& logicalPage) :
		mLogicalPage(logicalPage)
	{
		assert(mLogicalPage.VirtualTexturePtr != nullptr && "ERROR: A GlobalTexture modification request was given a VirtualTextureLogicalPage instance whose VirtualTexturePtr field was nullptr!");

		mLogicalPage.VirtualTexturePtr->IncrementStreamingRequestCount();
	}

	VirtualTextureStreamingNotifier::~VirtualTextureStreamingNotifier()
	{
		TryDecrementStreamingRequestCount();
	}

	VirtualTextureStreamingNotifier::VirtualTextureStreamingNotifier(VirtualTextureStreamingNotifier&& rhs) noexcept :
		mLogicalPage(rhs.mLogicalPage)
	{
		rhs.mLogicalPage = VirtualTextureLogicalPage{};
	}

	VirtualTextureStreamingNotifier& VirtualTextureStreamingNotifier::operator=(VirtualTextureStreamingNotifier&& rhs) noexcept
	{
		TryDecrementStreamingRequestCount();

		mLogicalPage = rhs.mLogicalPage;
		rhs.mLogicalPage = VirtualTextureLogicalPage{};

		return *this;
	}

	const VirtualTextureLogicalPage& VirtualTextureStreamingNotifier::GetLogicalPage() const
	{
		return mLogicalPage;
	}

	void VirtualTextureStreamingNotifier::SetVirtualTextureFirstUseableFrameNumber(const std::uint64_t frameNumber) const
	{
		if (mLogicalPage.VirtualTexturePtr != nullptr) [[likely]]
			mLogicalPage.VirtualTexturePtr->SetFirstUseableFrameNumber(frameNumber);
	}

	void VirtualTextureStreamingNotifier::TryDecrementStreamingRequestCount()
	{
		static constexpr std::uint64_t FRAME_NUMBER_DELTA = (Util::Engine::MAX_FRAMES_IN_FLIGHT + 1);
		
		if (mLogicalPage.VirtualTexturePtr != nullptr) [[likely]]
		{
			// We set the frame number for safe deletion to (Util::Engine::GetTrueFrameNumber() + (Util::Engine::MAX_FRAMES_IN_FLIGHT + 1)) 
			// to denote that when the true frame number reaches this value, then the VirtualTexture instance can safely
			// be deleted on the CPU timeline.
			//
			// We use (Util::Engine::MAX_FRAMES_IN_FLIGHT + 1), rather than just Util::Engine::MAX_FRAMES_IN_FLIGHT, so
			// that we guarantee that the deletion is safe regardless of whether the attempt to delete the VirtualTexture
			// instance happens before or after the FrameGraph fence wait in the Brawler Engine.
			mLogicalPage.VirtualTexturePtr->DecrementStreamingRequestCount(Util::Engine::GetTrueFrameNumber() + FRAME_NUMBER_DELTA);

			mLogicalPage = VirtualTextureLogicalPage{};
		}
	}
}