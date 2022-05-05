module;
#include <array>
#include "Win32Def.h"

module Brawler.AudioManager;

namespace Brawler
{
	AudioManager::AudioManager() :
		mXAudio2(nullptr),
		mMasterVoice(nullptr)
	{}

	void AudioManager::Initialize()
	{
		// Initialize XAudio2.
		CheckHRESULT(XAudio2Create(&mXAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR));

		// Create the mastering voice. The mastering voice encapsulates an audio device. It is
		// the ultimate destination for all audio that passes through an audio graph.
		IXAudio2MasteringVoice* masterVoice = nullptr;
		CheckHRESULT(mXAudio2->CreateMasteringVoice(&masterVoice));

		mMasterVoice = XAudio2Voice<IXAudio2MasteringVoice>{ masterVoice };
	}

	IXAudio2& AudioManager::GetXAudio2Engine()
	{
		return *(mXAudio2.Get());
	}

	const IXAudio2& AudioManager::GetXAudio2Engine() const
	{
		return *(mXAudio2.Get());
	}
}