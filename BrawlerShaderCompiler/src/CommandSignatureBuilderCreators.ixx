module;

export module Brawler.CommandSignatureBuilderCreators;
import Brawler.CommandSignatureID;
import Brawler.CommandSignatureBuilder;

export namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		CommandSignatureBuilder<CSIdentifier> CreateCommandSignatureBuilder();
	}
}