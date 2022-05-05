module;
#include <memory>

export module Brawler.StreamedAudio;
import Brawler.AssetSystem;
import Brawler.ThreadSafeQueue;

export namespace Brawler
{
	class StreamedAudioVoice;
	class AudioStreamingContext;
}

namespace IMPL
{
	static constexpr std::size_t REQUEST_QUEUE_SIZE = 16;

	struct AudioDataRequest
	{
		Brawler::AudioStreamingContext& StreamingContext;
		std::size_t RequestSize;
	};
}

export namespace Brawler
{
	class StreamedAudio final : public I_Asset
	{
	public:
		explicit StreamedAudio(FilePathHash&& pathHash);

		StreamedAudio(const StreamedAudio& rhs) = delete;
		StreamedAudio& operator=(const StreamedAudio& rhs) = delete;

		StreamedAudio(StreamedAudio&& rhs) noexcept = default;
		StreamedAudio& operator=(StreamedAudio&& rhs) noexcept = default;

		void UpdateAssetData() override;
		void ExecuteAssetDataLoad(const AssetDataLoadContext& context) override;
		void ExecuteAssetDataUnload(const AssetDataUnloadContext& context) override;
		bool IsLoaded() const override;
		AssetTypeID GetAssetTypeID() const override;

		void RequestAudioDataLoad(AudioStreamingContext& streamingContext, const std::size_t numBytesToLoad);

	private:
		ThreadSafeQueue<std::unique_ptr<IMPL::AudioDataRequest>, IMPL::REQUEST_QUEUE_SIZE> mPendingLoadRequestQueue;
		ThreadSafeQueue<std::unique_ptr<IMPL::AudioDataRequest>, IMPL::REQUEST_QUEUE_SIZE> mLoadRequestQueue;
	};
}