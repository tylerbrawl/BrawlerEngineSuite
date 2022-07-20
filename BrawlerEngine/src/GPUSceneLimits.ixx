module;
#include <cstddef>

export module Brawler.GPUSceneLimits;

export namespace Brawler
{
	// These values should match those from GPUSceneLimits.hlsli.
	//
	// NOTE: When adding new values here, make sure to shift static_cast<std::size_t>(1)
	// to avoid integer wraparound!

	constexpr std::size_t MAX_VERTEX_BUFFER_ELEMENTS = (static_cast<std::size_t>(1) << 22);
	constexpr std::size_t MAX_INDEX_BUFFER_ELEMENTS = (static_cast<std::size_t>(1) << 22);
	constexpr std::size_t MAX_MODEL_INSTANCES = (static_cast<std::size_t>(1) << 16);
	constexpr std::size_t MAX_MATERIAL_DEFINITIONS = (static_cast<std::size_t>(1) << 22);
	constexpr std::size_t MAX_VIEWS = (static_cast<std::size_t>(1) << 12);
	constexpr std::size_t MAX_TRIANGLE_CLUSTERS = (static_cast<std::size_t>(1) << 25);

	// There's no point in having more virtual texture descriptions than we do bindless
	// SRVs.
	constexpr std::size_t MAX_VIRTUAL_TEXTURE_DESCRIPTIONS = 500000;
}