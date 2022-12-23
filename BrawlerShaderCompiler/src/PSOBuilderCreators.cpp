module;
#include <compare>

module Brawler.PSOBuilderCreators;

namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		PSOBuilder<PSOIdentifier> CreatePSOBuilder()
		{
			static_assert(sizeof(PSOIdentifier) != sizeof(PSOIdentifier), "ERROR: An explicit template specialization of Brawler::PSOs::CreatePSOBuilder() was never created for a particular PSOID!");
		}
	}
}