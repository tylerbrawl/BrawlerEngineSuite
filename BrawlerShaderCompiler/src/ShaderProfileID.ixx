module;

export module Brawler.ShaderProfileID;

export namespace Brawler
{
	namespace ShaderProfiles
	{
		enum class ShaderProfileID
		{
			BRAWLER_ENGINE,
			MODEL_EXPORTER,
			TEST_SHADER_PROFILE,

			COUNT_OR_ERROR
		};
	}
}