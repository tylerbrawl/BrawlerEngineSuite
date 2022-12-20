module;

export module Brawler.CommandSignatures.CommandSignatureDefinition:CommandSignatureDefinitionBase;
import Brawler.CommandSignatures.CommandSignatureID;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		struct CommandSignatureDefinition
		{};
	}
}