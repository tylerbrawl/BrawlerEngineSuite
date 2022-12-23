// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;
#include <optional>
#include "../DxDef.h"

export module Brawler.CommandSignatures.CommandSignatureDefinition;
import :CommandSignatureDefinitionBase;
export import :CommandSignatureDefinition_DEFERRED_GEOMETRY_RASTER;

import Brawler.CommandSignatures.CommandSignatureID;
import Brawler.RootSignatures.RootSignatureID;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		using CommandSignatureType = typename CommandSignatureDefinition<CSIdentifier>::CommandSignatureType;

		template <CommandSignatureID CSIdentifier>
		consteval D3D12_COMMAND_SIGNATURE_DESC CreateCommandSignatureDescription()
		{
			return CommandSignatureDefinition<CSIdentifier>::COMMAND_SIGNATURE_DESCRIPTION;
		}

		template <CommandSignatureID CSIdentifier>
		consteval std::optional<RootSignatures::RootSignatureID> GetRootSignatureForCommandSignature()
		{
			constexpr RootSignatures::RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = CommandSignatureDefinition<CSIdentifier>::ASSOCIATED_ROOT_SIGNATURE_ID;

			if constexpr (ASSOCIATED_ROOT_SIGNATURE_ID == RootSignatures::RootSignatureID::COUNT_OR_ERROR)
				return {};
			else
				return ASSOCIATED_ROOT_SIGNATURE_ID;
		}
	}
}