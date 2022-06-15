module;
#include <compare>
#include <filesystem>
#include <vector>

export module Brawler.BCAInfoParsing.BCAInfoParser;
import Brawler.BCAInfo;
import Brawler.BCAInfoParsing.BCAInfoParserContext;
import Brawler.BCAInfoParsing.SourceAssetInfoParser;

export namespace Brawler
{
	namespace BCAInfoParsing
	{
		class BCAInfoParser
		{
		public:
			BCAInfoParser() = default;

			BCAInfoParser(const BCAInfoParser& rhs) = delete;
			BCAInfoParser& operator=(const BCAInfoParser& rhs) = delete;

			BCAInfoParser(BCAInfoParser&& rhs) noexcept = default;
			BCAInfoParser& operator=(BCAInfoParser&& rhs) noexcept = default;

			bool ParseBCAInfoFile(std::filesystem::path&& bcaInfoFilePath);

			void UpdateBCAInfoDatabase() const;
			void PrintErrorMessages() const;

		private:
			bool ParseSourceAssetInfoList();

		private:
			BCAInfoParserContext mParserContext;
			std::vector<SourceAssetInfoParser> mAssetInfoParserArr;
		};
	}
}