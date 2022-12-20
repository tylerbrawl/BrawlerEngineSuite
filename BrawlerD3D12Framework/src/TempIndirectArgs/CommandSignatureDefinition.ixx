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
		using IndirectArgumentsType = typename CommandSignatureDefinition<CSIdentifier>::IndirectArgumentsType;

		template <CommandSignatureID CSIdentifier>
		consteval D3D12_COMMAND_SIGNATURE_DESC CreateCommandSignatureDescription()
		{
			return CommandSignatureDefinition<CSIdentifier>::COMMAND_SIGNATURE_DESCRIPTION;
		}

		template <CommandSignatureID CSIdentifier>
		consteval bool DoesCommandSignatureHaveAssociatedRootSignature()
		{
			constexpr std::optional<RootSignatures::RootSignatureID> ASSOCIATED_ROOT_SIGNATURE_ID{ CommandSignatureDefinition<CSIdentifier>::ASSOCIATED_ROOT_SIGNATURE_ID };
			return ASSOCIATED_ROOT_SIGNATURE_ID.has_value();
		}

		template <CommandSignatureID CSIdentifier>
			requires DoesCommandSignatureHaveAssociatedRootSignature<CSIdentifier>()
		consteval RootSignatures::RootSignatureID GetRootSignatureForCommandSignature()
		{
			constexpr std::optional<RootSignatures::RootSignatureID> ASSOCIATED_ROOT_SIGNATURE_ID{ CommandSignatureDefinition<CSIdentifier>::ASSOCIATED_ROOT_SIGNATURE_ID };
			return *ASSOCIATED_ROOT_SIGNATURE_ID;
		}
	}
}