module;
#include <optional>
#include <string_view>

export module Brawler.CommandSignatureDefinition;
import Brawler.CommandSignatureID;
import Brawler.RootSignatureID;


namespace Brawler
{
	template <CommandSignatureID CSIdentifier>
	struct CommandSignatureDefinition
	{
		static_assert(sizeof(CSIdentifier) != sizeof(CSIdentifier), "ERROR: An explicit template specialization of Brawler::CommandSignatureDefinition was never provided for this CommandSignatureID! (See CommandSignatureDefinition.ixx.)");
	};

	// As of writing this, storing static constexpr std::optional<RootSignatureID> instances directly
	// within CommandSignatureDefinition template specializations is causing internal compiler errors.
	// 
	// So, we store the RootSignatureID directly and check its value; if the value assigned to
	// ASSOCIATED_ROOT_SIGNATURE_ID is RootSignatureID::COUNT_OR_ERROR, then it is implied that the
	// command signature is not associated with a root signature.

	template <>
	struct CommandSignatureDefinition<CommandSignatureID::DEFERRED_GEOMETRY_RASTER>
	{
		static constexpr std::string_view COMMAND_SIGNATURE_ID_STRING{ "DEFERRED_GEOMETRY_RASTER" };
		static constexpr RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = RootSignatureID::COUNT_OR_ERROR;
	};
}

export namespace Brawler
{
	template <CommandSignatureID CSIdentifier>
	consteval std::string_view GetCommandSignatureIDString()
	{
		return CommandSignatureDefinition<CSIdentifier>::COMMAND_SIGNATURE_ID_STRING;
	}

	template <CommandSignatureID CSIdentifier>
	consteval std::optional<RootSignatureID> GetRootSignatureForCommandSignature()
	{
		constexpr RootSignatureID ASSOCIATED_ROOT_SIGNATURE_ID = CommandSignatureDefinition<CSIdentifier>::ASSOCIATED_ROOT_SIGNATURE_ID;

		if constexpr (ASSOCIATED_ROOT_SIGNATURE_ID == RootSignatureID::COUNT_OR_ERROR)
			return {};
		else
			return std::optional<RootSignatureID>{ ASSOCIATED_ROOT_SIGNATURE_ID };
	}
}