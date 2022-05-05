module;
#include <array>
#include <string>
#include <ranges>
#include <ostream>
#include <istream>

export module Brawler.FileAttributes.BTEX;

export namespace Brawler
{
	namespace FileAttributes
	{
		namespace BTEX
		{
			/*
			Brawler Texture File (BTEX) Format
			Documentation - Version 1

			First, a CommonBTEXHeader is appropriately filled out and written to the file. Then, a
			VersionedBTEXHeaderV1 is written out to the file. The DDSFileSize member is the size of
			the DDS texture contained within the file; this will be discussed shortly.

			After the headers, a D3D12_RESOURCE_DESC1 (equivalent to Brawler::D3D12_RESOURCE_DESC) is
			written out which describes the texture. This resource description can be used to create
			the corresponding resource with the DirectX 12 API.

			Immediately after that comes the DDS file data. The size of this file is given in the
			aforementioned VersionedBTEXHeaderV1 header as the DDSFileSize member.
			*/

			constexpr std::string_view MAGIC = "BTEX";
			constexpr std::uint32_t CURRENT_VERSION = 1;

			struct CommonBTEXHeader
			{
				std::array<char, 4> Magic;
				std::uint32_t Version;
			};

			struct VersionedBTEXHeaderV1
			{
				std::size_t DDSFileSize;
			};

			using CurrentVersionedBTEXHeader = VersionedBTEXHeaderV1;

			consteval CommonBTEXHeader CreateCommonHeader()
			{
				CommonBTEXHeader commonHeader{
					.Magic{},
					.Version = CURRENT_VERSION
				};

				for (std::size_t i = 0; i < MAGIC.size(); ++i)
					commonHeader.Magic[i] = MAGIC[i];

				return commonHeader;
			}
		}
	}
}

export
{
	std::ostream& operator<<(std::ostream& lhs, const Brawler::FileAttributes::BTEX::CommonBTEXHeader& rhs)
	{
		lhs.write(reinterpret_cast<const char*>(&rhs), sizeof(rhs));

		return lhs;
	}

	std::istream& operator>>(std::istream& lhs, Brawler::FileAttributes::BTEX::CommonBTEXHeader& rhs)
	{
		lhs.read(reinterpret_cast<char*>(&rhs), sizeof(rhs));

		return lhs;
	}

	std::ostream& operator<<(std::ostream& lhs, const Brawler::FileAttributes::BTEX::VersionedBTEXHeaderV1& rhs)
	{
		lhs.write(reinterpret_cast<const char*>(&rhs), sizeof(rhs));

		return lhs;
	}

	std::istream& operator>>(std::istream& lhs, Brawler::FileAttributes::BTEX::VersionedBTEXHeaderV1& rhs)
	{
		lhs.read(reinterpret_cast<char*>(&rhs), sizeof(rhs));

		return lhs;
	}
}