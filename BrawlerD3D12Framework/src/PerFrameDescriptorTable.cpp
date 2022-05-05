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
#ifdef _DEBUG
			I_DescriptorTable(std::move(initInfo.HandleInfo)),
			mCreationFrameNum(initInfo.CurrentFrameNumber)
#else
			I_DescriptorTable(std::move(initInfo.HandleInfo))
#endif
		{}

		bool PerFrameDescriptorTable::IsDescriptorTableValid() const
		{
#ifdef _DEBUG
			return (mCreationFrameNum == Util::Engine::GetCurrentFrameNumber());
#else
			return true;
#endif // _DEBUG
		}
	}
}