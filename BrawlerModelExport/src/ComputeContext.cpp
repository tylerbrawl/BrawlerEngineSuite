module;
#include <functional>
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

		void ComputeContext::Dispatch(const std::uint32_t threadGroupCountX, const std::uint32_t threadGroupCountY, const std::uint32_t threadGroupCountZ)
		{
			GetCommandList().Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
		}
	}
}