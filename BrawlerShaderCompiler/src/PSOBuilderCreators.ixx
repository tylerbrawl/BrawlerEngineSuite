module;
#include <utility>
#include "DxDef.h"

export module Brawler.PSOBuilderCreators;
import Brawler.AppParams;  // Classic MSVC modules jank...
import Brawler.PSOID;
import Brawler.PSOBuilder;
import Brawler.PSOShaderFieldResolver;
import Brawler.ShaderCompilationParams;

/*
For information regarding some of the fields of a typical PSO, read the following MSDN articles *CAREFULLY*:

https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-blend-state
https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage

Additional Notes:
	- Whenever the MSDN refers to a "source" and "destination" for render target
	  outputs, the "source" is the value returned by interpolation or the pixel shader,
	  and the "destination" is the render target.
*/

namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		PSOBuilder<PSOIdentifier> CreateGeneralComputePSOBuilder(Brawler::ShaderCompilationParams&& compileParams)
		{
			Brawler::PSOShaderFieldResolver<CD3DX12_PIPELINE_STATE_STREAM_CS> computeShaderResolver{ std::move(compileParams) };

			PSOBuilder<PSOIdentifier> psoBuilder{};
			psoBuilder.AddPSOFieldResolver(std::move(computeShaderResolver));

			return psoBuilder;
		}
	}
}

export namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		PSOBuilder<PSOIdentifier> CreatePSOBuilder();
	}
}