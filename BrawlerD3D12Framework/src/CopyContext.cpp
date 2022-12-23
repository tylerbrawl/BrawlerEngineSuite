module;
#include <functional>

module Brawler.D3D12.CopyContext;

namespace Brawler
{
	namespace D3D12
	{
		void CopyContext::RecordCommandListIMPL(const std::function<void(CopyContext&)>& recordJob)
		{
			recordJob(*this);
		}

		void CopyContext::PrepareCommandListIMPL()
		{}
	}
}