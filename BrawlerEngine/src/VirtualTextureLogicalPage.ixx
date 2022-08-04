module;
#include <cstdint>
#include <span>

export module Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;

export namespace Brawler
{
	struct VirtualTextureLogicalPage
	{
		VirtualTexture* VirtualTexturePtr;
		std::uint32_t LogicalMipLevel;
		std::uint32_t LogicalPageXCoordinate;
		std::uint32_t LogicalPageYCoordinate;
	};
}

// ---------------------------------------------------------------------------------------------------------

export namespace std
{
	template <>
	struct hash<Brawler::VirtualTextureLogicalPage>
	{
		std::size_t operator()(const Brawler::VirtualTextureLogicalPage& key) const noexcept
		{
			// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...
			
			std::size_t hashValue = 5381;
			const std::span<const std::byte> keyByteSpan{ std::as_bytes(std::span<const Brawler::VirtualTextureLogicalPage>{ &key, 1 }) };

			for (const auto byte : keyByteSpan)
				hashValue = (((hashValue << 5) + hashValue) ^ byte);

			return hashValue;
		}
	};
}