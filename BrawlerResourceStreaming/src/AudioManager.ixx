module;
#include <array>
#include "Win32Def.h"

export module Brawler.AudioManager;
import Brawler.XAudio2Voice;

export namespace Brawler
{
	class AudioManager
	{
	public:
		AudioManager();

		AudioManager(const AudioManager& rhs) = delete;
		AudioManager& operator=(const AudioManager& rhs) = delete;

		AudioManager(AudioManager&& rhs) noexcept = default;
		AudioManager& operator=(AudioManager&& rhs) noexcept = default;

		void Initialize();

		IXAudio2& GetXAudio2Engine();
		const IXAudio2& GetXAudio2Engine() const;

	private:
		Microsoft::WRL::ComPtr<IXAudio2> mXAudio2;
		Brawler::XAudio2Voice<IXAudio2MasteringVoice> mMasterVoice;
	};
}