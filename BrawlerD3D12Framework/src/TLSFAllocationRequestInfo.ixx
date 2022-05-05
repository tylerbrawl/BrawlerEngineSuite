module;
#include <cstddef>

export module Brawler.D3D12.TLSFAllocationRequestInfo;

export namespace Brawler
{
	namespace D3D12
	{
		struct TLSFAllocationRequestInfo
		{
			std::size_t SizeInBytes;
			std::size_t Alignment;
		};
	}
}