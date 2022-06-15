module;
#include <string>
#include <vector>
#include <filesystem>

export module Brawler.BCAInfoParsing.SourceAssetInfoParser;
import Brawler.BCAInfoParsing.BCAInfoParserContext;
import Brawler.BCAInfoParsing.AttributeParser;
import Brawler.BCAInfo;

export namespace Brawler
{
	namespace BCAInfoParsing
	{
		class SourceAssetInfoParser
		{
		public:
			SourceAssetInfoParser() = default;

			SourceAssetInfoParser(const SourceAssetInfoParser& rhs) = delete;
			SourceAssetInfoParser& operator=(const SourceAssetInfoParser& rhs) = delete;

			SourceAssetInfoParser(SourceAssetInfoParser&& rhs) noexcept = default;
			SourceAssetInfoParser& operator=(SourceAssetInfoParser&& rhs) noexcept = default;

			bool ParseSourceAssetInfo(BCAInfoParserContext& parserContext);

			BCAInfo CreateBCAInfoForSourceAsset() const;
			const std::filesystem::path& GetSourceAssetFilePath() const;

		private:
			bool ParseFileName(BCAInfoParserContext& parserContext);
			bool ParseObjectDefinition(BCAInfoParserContext& parserContext);
			bool ParseAttributeList(BCAInfoParserContext& parserContext);

		private:
			std::string mAssetFileName;
			std::filesystem::path mAssetFilePath;
			std::vector<AttributeParser> mAttributeParserArr;
		};
	}
}