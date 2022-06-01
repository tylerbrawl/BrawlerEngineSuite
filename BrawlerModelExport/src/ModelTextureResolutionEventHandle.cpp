module;
#include <cstdint>

module Brawler.ModelTextureResolutionEventHandle;
import Util.Engine;

namespace Brawler
{
	ModelTextureResolutionEventHandle::ModelTextureResolutionEventHandle() :
		mCreationFrameNum(Util::Engine::GetCurrentFrameNumber())
	{}

	bool ModelTextureResolutionEventHandle::IsEventComplete() const
	{
		return Util::Engine::GetCurrentFrameNumber() > (mCreationFrameNum + Util::Engine::MAX_FRAMES_IN_FLIGHT);
	}
}