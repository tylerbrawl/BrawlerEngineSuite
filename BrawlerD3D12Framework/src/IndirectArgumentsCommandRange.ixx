module;
#include <cstddef>

export module Brawler.D3D12.IndirectArgumentsCommandRange;

export namespace Brawler
{
	namespace D3D12
	{
		struct IndirectArgumentsCommandRange
		{
			std::size_t FirstCommand;
			std::size_t NumCommands;
		};
	}
}