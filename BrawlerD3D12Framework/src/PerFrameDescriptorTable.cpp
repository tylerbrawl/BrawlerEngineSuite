module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.PerFrameDescriptorTable;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		PerFrameDescriptorTable::PerFrameDescriptorTable(InitializationInfo&& initInfo) :
			I_DescriptorTable(std::move(initInfo.HandleInfo)),
			mCreationFrameNum(initInfo.CurrentFrameNumber)
		{}

		bool PerFrameDescriptorTable::IsDescriptorTableValid() const
		{
			return (mCreationFrameNum == Util::Engine::GetCurrentFrameNumber());
		}
	}
}