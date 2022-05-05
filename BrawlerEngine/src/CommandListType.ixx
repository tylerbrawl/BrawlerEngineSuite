module;
#include <cassert>
#include "DxDef.h"

export module Brawler.CommandListType;

export namespace Brawler
{
	enum class CommandListType
	{
		DIRECT,
		COMPUTE,
		COPY,

		COUNT_OR_ERROR
	};

	constexpr D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(const Brawler::CommandListType cmdListType)
	{
		switch (cmdListType)
		{
		case Brawler::CommandListType::DIRECT:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;

		case Brawler::CommandListType::COMPUTE:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;

		case Brawler::CommandListType::COPY:
			return D3D12_COMMAND_LIST_TYPE_COPY;

		default:
			assert(false && "ERROR: An attempt was made to get information on an undefined Brawler::CommandListType in Brawler::GetD3D12CommandListType()!");
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		}
	}
}