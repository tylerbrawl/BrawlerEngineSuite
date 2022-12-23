module;

export module Brawler.RootSignatureBuilderCreators;
import Brawler.RootSignatureID;
import Brawler.RootSignatureBuilder;

export namespace Brawler
{
	namespace RootSignatures
	{
		template <Brawler::RootSignatureID RSIdentifier>
		RootSignatureBuilder<RSIdentifier> CreateRootSignatureBuilder();
	}
}