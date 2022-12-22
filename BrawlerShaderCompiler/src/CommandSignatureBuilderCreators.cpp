module;

module Brawler.CommandSignatureBuilderCreators;
import Brawler.IndirectArgumentResolvers;

namespace Brawler
{
	namespace CommandSignatures
	{
		template <CommandSignatureID CSIdentifier>
		CommandSignatureBuilder<CSIdentifier> CreateCommandSignatureBuilder()
		{
			static_assert(sizeof(CSIdentifier) != sizeof(CSIdentifier), "ERROR: An explicit template specialization of Brawler::CommandSignatures::CreateCommandSignatureBuilder() was never provided for this Brawler::CommandSignatureID value! (See CommandSignatureBuilderCreators.cpp.)");
		}

		template <>
		CommandSignatureBuilder<CommandSignatureID::DEFERRED_GEOMETRY_RASTER> CreateCommandSignatureBuilder<CommandSignatureID::DEFERRED_GEOMETRY_RASTER>()
		{
			CommandSignatureBuilder<CommandSignatureID::DEFERRED_GEOMETRY_RASTER> cmdSignatureBuilder{};

			// Add a D3D12_DRAW_ARGUMENTS field named "DrawArguments" to the command signature. This will
			// be indirect argument 0.
			cmdSignatureBuilder.AddIndirectArgument(DrawInstancedIndirectArgumentResolver{ "DrawArguments" });

			// Since that's the only indirect argument in the command signature, we are finished.
			return cmdSignatureBuilder;
		}
	}
}