module;
#include <filesystem>
#include <string>
#include <format>
#include <cassert>

module Brawler.BCAInfoEntry;
import Brawler.NZStringView;

namespace
{
	static constexpr Brawler::NZStringView DO_NOT_COMPRESS_STR{ "DoNotCompress" };
	static constexpr bool DEFAULT_DO_NOT_COMPRESS_VALUE = false;

	template <typename T>
	Brawler::NZStringView GetValueText(const T value)
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: No explicit specialization was provided for GetValueText() for a given type of attribute within BCAInfoEntry.cpp!");
	}

	template <>
	Brawler::NZStringView GetValueText(const bool value)
	{
		static constexpr Brawler::NZStringView TRUE_WIN32_STR{ "TRUE" };
		static constexpr Brawler::NZStringView FALSE_WIN32_STR{ "FALSE" };

		return (value ? TRUE_WIN32_STR : FALSE_WIN32_STR);
	}
}

namespace Brawler
{
	BCAInfoEntry::BCAInfoEntry(std::filesystem::path&& associatedFilePath) :
		mFilePath(std::move(associatedFilePath)),
		mDoNotCompress(DEFAULT_DO_NOT_COMPRESS_VALUE)
	{}

	void BCAInfoEntry::SetCompressionStatus(const bool enableCompression)
	{
		mDoNotCompress = !enableCompression;
	}

	const std::filesystem::path& BCAInfoEntry::GetAssociatedFilePath() const
	{
		return mFilePath;
	}

	std::string BCAInfoEntry::GetEntryText() const
	{
		/*
		For future reference, here is what an example entry looks like for a file (including its file
		extension) named FileName:

		"FileName" :
		{
			"DoNotCompress" : TRUE  // Attributes are separated by commas, should additional attributes be added.
		}

		BCAINFO files use a pretty similar format to JSON, but the parsing rules are even simpler. For
		the complete BCAINFO context-free grammar (CFG), check out BCAInfoParser.cpp in the
		BrawlerFilePacker project.
		*/

		assert(!mFilePath.empty() && "ERROR: A BCAInfoEntry instance was never assigned a valid std::filesystem::path instance!");

		return std::format(
			R"("{}" :
{{
	"DoNotCompress": {}
}})",
			mFilePath.filename().string(),
			GetValueText(mDoNotCompress).C_Str()
		);
	}
}