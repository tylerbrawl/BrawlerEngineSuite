module;
#include <cstdint>
#include <span>

export module Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;
import Brawler.GeneralHash;

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
			const Brawler::GeneralHash<Brawler::VirtualTextureLogicalPage> hasher{};
			return hasher(key);
		}
	};
}

export namespace Brawler
{
	bool operator==(const VirtualTextureLogicalPage& lhs, const VirtualTextureLogicalPage& rhs)
	{
		return (lhs.VirtualTexturePtr == rhs.VirtualTexturePtr && lhs.LogicalMipLevel == rhs.LogicalMipLevel && lhs.LogicalPageXCoordinate == rhs.LogicalPageXCoordinate &&
			lhs.LogicalPageYCoordinate == rhs.LogicalPageYCoordinate);
	}
}