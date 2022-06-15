module;
#include <cstddef>

export module Brawler.D3D12.StructuredBufferSubAllocation:StructuredBufferElementRange;

export namespace Brawler
{
	namespace D3D12
	{
		struct StructuredBufferElementRange
		{
			std::size_t FirstElement;
			std::size_t NumElements;
		};
	}
}