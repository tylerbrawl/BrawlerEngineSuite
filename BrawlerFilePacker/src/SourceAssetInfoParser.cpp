module;
#include <string>
#include <format>
#include <optional>
#include <cassert>
#include <filesystem>
#include <vector>

module Brawler.BCAInfoParsing.SourceAssetInfoParser;
import Util.General;

namespace Brawler
{
	namespace BCAInfoParsing
	{
		bool SourceAssetInfoParser::ParseSourceAssetInfo(BCAInfoParserContext& parserContext)
		{
			if (!ParseFileName(parserContext)) [[unlikely]]
				return false;

			if (!parserContext.Expect(":")) [[unlikely]]
			{
				parserContext.AddErrorString(std::format(LR"(SYNTAX ERROR: A colon (':') is expected after the file name "{}"!)", Util::General::StringToWString(mAssetFileName)));
				return false;
			}

			return ParseObjectDefinition(parserContext);
		}

		BCAInfo SourceAssetInfoParser::CreateBCAInfoForSourceAsset() const
		{
			BCAInfo bcaInfo{ DEFAULT_BCA_INFO_VALUE };

			for (const auto& attributeParser : mAttributeParserArr)
				attributeParser.ResolveBCAInfo(bcaInfo);

			return bcaInfo;
		}

		const std::filesystem::path& SourceAssetInfoParser::GetSourceAssetFilePath() const
		{
			return mAssetFilePath;
		}

		bool SourceAssetInfoParser::ParseFileName(BCAInfoParserContext& parserContext)
		{
			if (!parserContext.Expect(R"(")")) [[unlikely]]
			{
				parserContext.AddErrorString(LR"(SYNTAX ERROR: A double quotes ('"') character is expected before giving a file name!)");
				return false;
			}

			std::string fileNameStr{};

			{
				ScopedWhiteSpaceSkipToggle disableWhiteSpaceSkipping{ parserContext };

				// Consume characters until we find the matching ending quotes.
				while (true)
				{
					const std::optional<std::uint8_t> currChar = parserContext.Peek();
					parserContext.Consume();

					if (!currChar.has_value()) [[unlikely]]
					{
						parserContext.AddErrorString(LR"(SYNTAX ERROR: A double quotes ('"') character is expected at the end of a file name!)");
						return false;
					}

						if (*currChar == '"')
							break;
						else
							fileNameStr += *currChar;
				}
			}

			if (fileNameStr.empty()) [[unlikely]]
			{
				parserContext.AddErrorString(L"ERROR: File names cannot be empty!");
				return false;
			}

			// Ensure that the provided file name refers to a file in the same directory as that of
			// the .BCAINFO file.
			const std::filesystem::path bcaInfoParentDir{ parserContext.GetBCAInfoFilePath().parent_path() };
			std::filesystem::path proposedCurrentFilePath{ bcaInfoParentDir / std::filesystem::path{fileNameStr} };
			{
				std::error_code errorCode{};
				const bool fileExistsInDirectory = std::filesystem::exists(proposedCurrentFilePath, errorCode);

				if (errorCode) [[unlikely]]
				{
					parserContext.AddErrorString(std::format(
						LR"(INTERNAL ASSET COMPILER ERROR: The attempt to verify whether or not the file "{}" exists in the directory "{}" failed with the following error: {})",
						Util::General::StringToWString(fileNameStr),
						bcaInfoParentDir.c_str(),
						Util::General::StringToWString(errorCode.message())
					));
					return false;
				}

				if (!fileExistsInDirectory) [[unlikely]]
				{
					parserContext.AddErrorString(std::format(LR"(ERROR: The file "{}" does not exist in the directory "{}!")", Util::General::StringToWString(fileNameStr), bcaInfoParentDir.c_str()));
					return false;
				}

				const bool isDirectory = std::filesystem::is_directory(proposedCurrentFilePath, errorCode);

				if (errorCode) [[unlikely]]
				{
					parserContext.AddErrorString(std::format(LR"(INTERNAL ASSET COMPILER ERROR: The attempt to check if the file "{}" was actually a directory failed with the following error: {})", Util::General::StringToWString(fileNameStr), Util::General::StringToWString(errorCode.message())));
					return false;
				}

				if (isDirectory) [[unlikely]]
				{
					parserContext.AddErrorString(std::format(LR"(ERROR: The file "{}" is actually a directory!)", Util::General::StringToWString(fileNameStr)));
					return false;
				}

				// Convert the path to an absolute path to ensure that it remains consistent
				// with the paths provided to BCAArchive instances.
				std::filesystem::path absPath = std::filesystem::absolute(proposedCurrentFilePath, errorCode);

				if (errorCode) [[unlikely]]
				{
					parserContext.AddErrorString(std::format(LR"(INTERNAL ASSET COMPILER ERROR: The attempt to get the absolute file path of the directory "{}" failed with the following error: {})", proposedCurrentFilePath.c_str(), Util::General::StringToWString(errorCode.message())));
					return false;
				}

				// The file appears to be valid.
				mAssetFileName = fileNameStr;
				mAssetFilePath = std::move(absPath);
			}

			return true;
		}

		bool SourceAssetInfoParser::ParseObjectDefinition(BCAInfoParserContext& parserContext)
		{
			if (!parserContext.Expect("{")) [[unlikely]]
			{
				parserContext.AddErrorString(L"ERROR: A beginning brace ('{') character is expected before giving an attribute list!");
				return false;
			}

			if (!ParseAttributeList(parserContext)) [[unlikely]]
				return false;

			// This should never fire due to checks done in SourceAssetInfoParser::ParseAttributeList().
			{
				const bool endAttributeList = parserContext.Expect("}");
				assert(endAttributeList);
			}

			return true;
		}

		bool SourceAssetInfoParser::ParseAttributeList(BCAInfoParserContext& parserContext)
		{
			enum class AttributeRuleElement
			{
				ATTRIBUTE_DEFINITION,
				COMMA
			};
			
			AttributeRuleElement allowedElement = AttributeRuleElement::ATTRIBUTE_DEFINITION;
			
			while (true)
			{
				const std::optional<std::uint8_t> nextChar = parserContext.Peek();

				if (!nextChar.has_value()) [[unlikely]]
				{
					parserContext.AddErrorString(L"ERROR: An ending brace ('}') character is expected at the end of an attribute list!");
					return false;
				}

				switch (*nextChar)
				{
				case '"':
				{
					if (allowedElement != AttributeRuleElement::ATTRIBUTE_DEFINITION) [[unlikely]]
					{
						parserContext.AddErrorString(L"SYNTAX ERROR: A comma (',') is expected between successive attribute name/value pairs! (NOTE: Much like with C++ enumerations, a redundant comma can be added to the end of the last name/value pair, but this is not required.");
						return false;
					}
					
					AttributeParser attributeParser{};

					if (!attributeParser.ParseAttribute(parserContext)) [[unlikely]]
						return false;

					allowedElement = AttributeRuleElement::COMMA;
					mAttributeParserArr.push_back(std::move(attributeParser));

					break;
				}

				case ',':
				{
					if (allowedElement != AttributeRuleElement::COMMA) [[unlikely]]
					{
						parserContext.AddErrorString(L"SYNTAX ERROR: A comma (',') is only allowed between successive attribute name/value pairs!");
						return false;
					}

					parserContext.Consume();
					allowedElement = AttributeRuleElement::ATTRIBUTE_DEFINITION;

					break;
				}

				case '}':
					return true;

				default: [[unlikely]]
				{
					parserContext.AddErrorString(LR"(SYNTAX ERROR: Attribute names must be surrounded by double quote ('"') characters!)");
					return false;
				}
				}
			}
		}
	}
}