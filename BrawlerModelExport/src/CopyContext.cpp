module;
#include <functional>
#include "DxDef.h"

module Brawler.D3D12.CopyContext;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		void CopyContext::RecordCommandListIMPL(const std::function<void(CopyContext&)>& recordJob)
		{
			recordJob(*this);
		}
	}
}