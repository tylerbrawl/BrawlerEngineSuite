module;
#include <cstddef>

export module Brawler.GPUSceneLimits;

export namespace Brawler
{
	namespace GPUSceneLimits
	{
		// These values should match those from GPUSceneLimits.hlsli.
		//
		// NOTE: When adding new values here, make sure to shift static_cast<std::size_t>(1)
		// to avoid integer wraparound!

		constexpr std::size_t MAX_MATERIAL_DEFINITIONS = (static_cast<std::size_t>(1) << 22);
		constexpr std::size_t MAX_MODEL_INSTANCES = (static_cast<std::size_t>(1) << 16);
		constexpr std::size_t MAX_MODELS = (static_cast<std::size_t>(1) << 15);
		constexpr std::size_t MAX_VIEWS = (static_cast<std::size_t>(1) << 12);
		constexpr std::size_t MAX_VERTEX_BUFFER_ELEMENTS = (static_cast<std::size_t>(1) << 22);

		// Until some form of light culling is implemented, MAX_LIGHTS is intentionally kept to
		// a pathetically low value.
		constexpr std::size_t MAX_LIGHTS = (static_cast<std::size_t>(1) << 8);
	}
}