module;
#include <optional>
#include <memory>
#include "Win32Def.h"

module Brawler.StreamedAudioVoice;
import Util.Engine;

namespace
{
	static constexpr std::uint32_t DESIRED_QUEUED_AUDIO_BUFFER_COUNT = 2;
}

namespace Brawler
{
	StreamedAudioVoice::StreamedAudioVoice(AssetHandle<StreamedAudio> hAudioAsset) :
		mSourceVoice(nullptr),
		mStreamingContext(*hAudioAsset),
		mVoiceStarted(false)
	{
		mStreamingContext.RequestAudioDataLoad(DESIRED_QUEUED_AUDIO_BUFFER_COUNT);
	}

	void StreamedAudioVoice::Update(const float dt)
	{
		if (mSourceVoice == nullptr)
			TryInitializeXAudio2Voice();

		if (mSourceVoice != nullptr)
			CheckAudioBuffers();
	}

	void StreamedAudioVoice::TryInitializeXAudio2Voice()
	{
		const std::optional<WAVEFORMATEXTENSIBLE>& waveFormatEx{ mStreamingContext.GetWaveFormat() };

		if (waveFormatEx.has_value())
		{
			IXAudio2SourceVoice* sourceVoice = nullptr;
			CheckHRESULT(Util::Engine::GetXAudio2Engine().CreateSourceVoice(&sourceVoice, reinterpret_cast<const WAVEFORMATEX*>(&(*waveFormatEx))));

			mSourceVoice = XAudio2Voice<IXAudio2SourceVoice>{ sourceVoice };
		}
	}

	void StreamedAudioVoice::CheckAudioBuffers()
	{
		XAUDIO2_VOICE_STATE voiceState{};
		mSourceVoice->GetState(&voiceState, XAUDIO2_VOICE_NOSAMPLESPLAYED);

		std::uint32_t buffersToQueue = DESIRED_QUEUED_AUDIO_BUFFER_COUNT - voiceState.BuffersQueued;

		// First, try filling in source buffers from the AudioStreamingContext.
		while (buffersToQueue > 0)
		{
			std::optional<XAUDIO2_BUFFER> xAudioBuffer{ mStreamingContext.GetXAudio2Buffer() };

			if (!xAudioBuffer.has_value())
				break;

			mSourceVoice->SubmitSourceBuffer(&(*xAudioBuffer));
			--buffersToQueue;

			if (!mVoiceStarted)
			{
				mSourceVoice->Start(0);
				mVoiceStarted = true;
			}
		}

		// If the AudioStreamingContext could not provide enough buffers, then request to
		// load them.
		if (buffersToQueue > 0)
			mStreamingContext.RequestAudioDataLoad(buffersToQueue);
	}
}