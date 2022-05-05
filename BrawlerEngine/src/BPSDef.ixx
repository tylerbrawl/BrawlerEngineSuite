module;
#include <string>
#include <array>
#include <fstream>
#include <filesystem>

export module Brawler.IMPL.BPSDef;

export namespace Brawler
{
	namespace IMPL
	{
		const std::filesystem::path PSO_CACHE_PATH{ std::filesystem::current_path() / L"PSO Cache" };
		static constexpr std::wstring_view BPS_EXTENSION = L".bps";
		
		static constexpr std::string_view BPS_MAGIC = "BPS";

		struct CommonBPSHeader
		{
			std::array<char, BPS_MAGIC.size() + 1> Magic;
			std::uint32_t Version;
		};

		static constexpr std::uint32_t CURRENT_BPS_VERSION = 1;

		struct VersionedBPSHeaderV1
		{
			/// <summary>
			/// This is the version number of the corresponding PSO. It is unrelated to the
			/// version number of the BPS versioned file header.
			/// </summary>
			std::uint32_t PSOVersion;

			/// <summary>
			/// This is the size, in bytes, of the cached PSO.
			/// </summary>
			std::size_t PSOBlobSizeInBytes;
		};

		using CurrentVersionedBPSHeader = VersionedBPSHeaderV1;
	}
}

export
{
	std::ifstream& operator>>(std::ifstream& lhs, Brawler::IMPL::CommonBPSHeader& rhs)
	{
		lhs.read(rhs.Magic.data(), rhs.Magic.size());
		lhs.read(reinterpret_cast<char*>(&(rhs.Version)), sizeof(rhs.Version));

		return lhs;
	}

	std::ofstream& operator<<(std::ofstream& lhs, const Brawler::IMPL::CommonBPSHeader& rhs)
	{
		lhs.write(rhs.Magic.data(), rhs.Magic.size());
		lhs.write(reinterpret_cast<const char*>(&(rhs.Version)), sizeof(rhs.Version));

		return lhs;
	}

	std::ifstream& operator>>(std::ifstream& lhs, Brawler::IMPL::VersionedBPSHeaderV1& rhs)
	{
		lhs.read(reinterpret_cast<char*>(&(rhs.PSOVersion)), sizeof(rhs.PSOVersion));
		lhs.read(reinterpret_cast<char*>(&(rhs.PSOBlobSizeInBytes)), sizeof(rhs.PSOBlobSizeInBytes));

		return lhs;
	}

	std::ofstream& operator<<(std::ofstream& lhs, const Brawler::IMPL::VersionedBPSHeaderV1& rhs)
	{
		lhs.write(reinterpret_cast<const char*>(&(rhs.PSOVersion)), sizeof(rhs.PSOVersion));
		lhs.write(reinterpret_cast<const char*>(&(rhs.PSOBlobSizeInBytes)), sizeof(rhs.PSOBlobSizeInBytes));

		return lhs;
	}
}