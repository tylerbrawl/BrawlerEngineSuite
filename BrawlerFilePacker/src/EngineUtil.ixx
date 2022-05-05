module;
#include <cstddef>
#include <cstdint>

export module Util.Engine;
import Brawler.PackerSettings;

export namespace Brawler
{
	class HashProvider;
}

export namespace Util
{
	namespace Engine
	{
		static constexpr std::size_t SHA_512_HASH_SIZE_IN_BYTES = 64;
		
		Brawler::HashProvider& GetHashProvider();
		
		Brawler::PackerSettings::BuildMode GetAssetBuildMode();
		std::int32_t GetZSTDCompressionLevel();
	}
}