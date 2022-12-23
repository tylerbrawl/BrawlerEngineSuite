module;

module Brawler.RootSignatureBuilderCreators;

namespace Brawler
{
	namespace RootSignatures
	{
		template <Brawler::RootSignatureID RSIdentifier>
		RootSignatureBuilder<RSIdentifier> CreateRootSignatureBuilder()
		{
			static_assert(sizeof(RSIdentifier) != sizeof(RSIdentifier), "ERROR: An explicit template specialization of Brawler::RootSignatures::CreateRootSignatureBuilder() was never provided for a particular RootSignatureID! (See RootSignatureBuilderCreators.ixx.)");
		}
	}
}