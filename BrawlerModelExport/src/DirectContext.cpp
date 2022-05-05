module;
#include <functional>

module Brawler.D3D12.DirectContext;

namespace Brawler
{
	namespace D3D12
	{
		void DirectContext::RecordCommandListIMPL(const std::function<void(DirectContext&)>& recordJob)
		{
			recordJob(*this);
		}
	}
}