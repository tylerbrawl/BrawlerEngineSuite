module;
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.ComputeContext;

namespace Brawler
{
	namespace D3D12
	{
		void ComputeContext::RecordCommandListIMPL(const std::function<void(ComputeContext&)>& recordJob)
		{
			recordJob(*this);
		}

		void ComputeContext::PrepareCommandListIMPL()
		{
			mCurrPSOID.reset();
		}
	}
}