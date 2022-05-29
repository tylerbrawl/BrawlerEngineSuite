module;
#include <filesystem>
#include <cassert>

module Brawler.BCAInfoParsing.BCAInfoParser;

/*
The context-free grammar (CFG) for .BCAINFO files is similar to the grammar of JSON files,
but is simplified. It is defined by the following list of rules:

CFG Rules:
bca-info -> source-asset-info-list | EPSILON
source-asset-info-list -> source-asset-info | source-asset-info source-asset-info-list
source-asset-info -> file-name COLON object-definition
file-name -> DOUBLE_QUOTES CHARACTER_SEQUENCE DOUBLE_QUOTES
object-definition -> BEGIN_BRACE attribute-list END_BRACE
attribute-list -> attribute | attribute COMMA attribute-list | EPSILON
attribute -> DOUBLE_QUOTES CHARACTER_SEQUENCE DOUBLE_QUOTES COLON attribute-definition
attribute-definition -> TRUE_WIN32 | FALSE_WIN32 | TRUE_CXX | FALSE_CXX

CFG Terminals:
COLON -> :
DOUBLE_QUOTES -> "
CHARACTER_SEQUENCE -> [Any Valid Characters Excepting Quotes]
BEGIN_BRACE -> {
END_BRACE -> }
COMMA -> :
EPSILON -> [Empty String]
TRUE_WIN32 -> TRUE
FALSE_WIN32 -> FALSE
TRUE_CXX -> true
FALSE_CXX -> false
*/

namespace Brawler
{
	namespace BCAInfoParsing
	{
		bool BCAInfoParser::ParseBCAInfoFile(std::filesystem::path&& bcaInfoFilePath)
		{
			mParserContext = BCAInfoParserContext{ std::move(bcaInfoFilePath) };

			return ParseSourceAssetInfoList();
		}

		bool BCAInfoParser::ParseSourceAssetInfoList()
		{
			while (!mParserContext.IsFinished())
			{
				SourceAssetInfoParser assetInfoParser{};

				if (!assetInfoParser.ParseSourceAssetInfo(mParserContext))
					return false;

				mAssetInfoParserArr.push_back(std::move(assetInfoParser));
			}

			return true;
		}
	}
}