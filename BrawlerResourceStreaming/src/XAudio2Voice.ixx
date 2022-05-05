module;
#include <memory>
#include "Win32Def.h"

export module Brawler.XAudio2Voice;

namespace IMPL
{
	template <typename T>
		requires std::derived_from<T, IXAudio2Voice>
	struct VoiceDeleter
	{
		void operator()(T* voice)
		{
			if (voice != nullptr)
				voice->DestroyVoice();
		}
	};
}

export namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, IXAudio2Voice>
	using XAudio2Voice = std::unique_ptr<T, IMPL::VoiceDeleter<T>>;
}