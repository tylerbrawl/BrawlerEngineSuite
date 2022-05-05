module;
#include <memory>
#include <optional>
#include <stdexcept>
#include <cassert>

module Brawler.StreamedAudio;
import Brawler.StreamedAudioVoice;
import Brawler.AudioStreamingContext;

namespace Brawler
{
	StreamedAudio::StreamedAudio(FilePathHash&& pathHash) :
		I_Asset(std::move(pathHash)),
		mPendingLoadRequestQueue(),
		mLoadRequestQueue()
	{}

	void StreamedAudio::UpdateAssetData()
	{
		std::optional<std::unique_ptr<IMPL::AudioDataRequest>> requestPtr{ mPendingLoadRequestQueue.TryPop() };
		while (requestPtr.has_value())
		{
			SubmitAssetDataLoadRequest((*requestPtr)->RequestSize, JobPriority::CRITICAL);

			if (!mLoadRequestQueue.PushBack(std::move(*requestPtr))) [[unlikely]]
				throw std::runtime_error{ "ERROR: The load request queue for audio streaming could not contain all of its requests!" };

			requestPtr = mPendingLoadRequestQueue.TryPop();
		}
	}

	void StreamedAudio::ExecuteAssetDataLoad(const AssetDataLoadContext& context)
	{
		// Since all of our requests are guaranteed to be executed due to their priority being JobPriority::CRITICAL,
		// it doesn't really matter if the size allowed specified by context matches what is at the front of the
		// queue.

		std::optional<std::unique_ptr<IMPL::AudioDataRequest>> requestPtr{ mLoadRequestQueue.TryPop() };
		assert(requestPtr.has_value());

		(*requestPtr)->StreamingContext.LoadAudioData((*requestPtr)->RequestSize);
	}

	void StreamedAudio::ExecuteAssetDataUnload(const AssetDataUnloadContext& context)
	{
		// Data unloading never really happens with streaming audio, since it is implemented as a ring buffer.
		// Thus, the allocated memory gets re-used.
	}

	bool StreamedAudio::IsLoaded() const
	{
		// Streamed audio is never finished loading.
		return false;
	}
	
	AssetTypeID StreamedAudio::GetAssetTypeID() const
	{
		return AssetTypeID::STREAMED_AUDIO;
	}

	void StreamedAudio::RequestAudioDataLoad(AudioStreamingContext& streamingContext, const std::size_t numBytesToLoad)
	{
		std::unique_ptr<IMPL::AudioDataRequest> loadRequest{ std::make_unique<IMPL::AudioDataRequest>(streamingContext, numBytesToLoad) };
		
		if (!mPendingLoadRequestQueue.PushBack(std::move(loadRequest))) [[unlikely]]
			throw std::runtime_error{ "ERROR: The load request queue for audio streaming could not contain all of its requests!" };
	}
}