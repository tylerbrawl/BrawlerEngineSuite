module;
#include "Win32Def.h"

export module Brawler.StreamedAudioVoice;
import Brawler.AssetSystem;
import Brawler.StreamedAudio;
import Brawler.AudioStreamingContext;
import Brawler.XAudio2Voice;

export namespace Brawler
{
	class StreamedAudioVoice
	{
	public:
		explicit StreamedAudioVoice(AssetHandle<StreamedAudio> hAudioAsset);

		StreamedAudioVoice(const StreamedAudioVoice& rhs) = delete;
		StreamedAudioVoice& operator=(const StreamedAudioVoice& rhs) = delete;

		StreamedAudioVoice(StreamedAudioVoice&& rhs) noexcept = default;
		StreamedAudioVoice& operator=(StreamedAudioVoice&& rhs) noexcept = default;

		void Update(const float dt);

	private:
		void TryInitializeXAudio2Voice();
		void CheckAudioBuffers();

	private:
		XAudio2Voice<IXAudio2SourceVoice> mSourceVoice;
		AudioStreamingContext mStreamingContext;
		bool mVoiceStarted;
	};
}