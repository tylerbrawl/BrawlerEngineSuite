module;
#include <unordered_map>
#include <string_view>
#include <cassert>
#include <string>
#include <optional>
#include <format>

module Brawler.BCAInfoParsing.AttributeParser;
import Util.General;

namespace
{
	static constexpr std::string_view DO_NOT_COMPRESS_ATTRIBUTE_NAME{ "DoNotCompress" };

	static constexpr std::string_view TRUE_WIN32_ATTRIBUTE_VALUE{ "TRUE" };
	static constexpr std::string_view TRUE_CXX_ATTRIBUTE_VALUE{ "true" };
	static constexpr std::string_view FALSE_WIN32_ATTRIBUTE_VALUE{ "FALSE" };
	static constexpr std::string_view FALSE_CXX_ATTRIBUTE_VALUE{ "false" };

	template <typename RetType, typename... Args>
	using FunctionPtr = Brawler::BCAInfoParsing::FunctionPtr<RetType, Args...>;

	template <bool DisableCompression>
	static void ToggleDoNotCompress(Brawler::BCAInfo& bcaInfo)
	{
		bcaInfo.DoNotCompress = DisableCompression;
	}
	
	static const std::unordered_map<std::string_view, FunctionPtr<bool, Brawler::BCAInfoParsing::BCAInfoParserContext&, FunctionPtr<void, Brawler::BCAInfo&>&>> attributeValueVerificationMap = [] ()
	{
		std::unordered_map<std::string_view, Brawler::BCAInfoParsing::FunctionPtr<bool, Brawler::BCAInfoParsing::BCAInfoParserContext&, FunctionPtr<void, Brawler::BCAInfo&>&>> verificationMap{};
		verificationMap[DO_NOT_COMPRESS_ATTRIBUTE_NAME] = [] (Brawler::BCAInfoParsing::BCAInfoParserContext& parserContext, FunctionPtr<void, Brawler::BCAInfo&>& bcaInfoResolverPtr)
		{
			if (parserContext.Expect(TRUE_WIN32_ATTRIBUTE_VALUE) || parserContext.Expect(TRUE_CXX_ATTRIBUTE_VALUE))
			{
				bcaInfoResolverPtr = ToggleDoNotCompress<true>;
				return true;
			}

			if (parserContext.Expect(FALSE_WIN32_ATTRIBUTE_VALUE) || parserContext.Expect(FALSE_CXX_ATTRIBUTE_VALUE))
			{
				bcaInfoResolverPtr = ToggleDoNotCompress<false>;
				return true;
			}

			parserContext.AddErrorString(L"ERROR: DoNotCompress is a boolean attribute, and must be set to either true/TRUE or false/FALSE!");
			return false;
		};

		return verificationMap;
	}();
}

namespace Brawler
{
	namespace BCAInfoParsing
	{
		bool AttributeParser::ParseAttribute(BCAInfoParserContext& parserContext)
		{
			// We shouldn't be creating AttributeParser instances unless we detect an attribute, so
			// this should never assert.
			{
				const bool attributeBeginning = parserContext.Expect(R"(")");
				assert(attributeBeginning);
			}

			std::string attributeName{};

			{
				ScopedWhiteSpaceSkipToggle disableWhiteSpaceSkipping{ parserContext };

				while (true)
				{
					const std::optional<std::uint8_t> nextChar = parserContext.Peek();
					parserContext.Consume();

					if (!nextChar.has_value())
					{
						parserContext.AddErrorString(LR"(SYNTAX ERROR: A double quotes ('"') character is expected at the end of an attribute name!)");
						return false;
					}

					if (*nextChar == '"')
						break;
					else
						attributeName += *nextChar;
				}
			}

			if (attributeName.empty()) [[unlikely]]
			{
				parserContext.AddErrorString(L"ERROR: Attribute names cannot be empty!");
				return false;
			}

			// Make sure that we are dealing with a recognized attribute.
			if (!attributeValueVerificationMap.contains(attributeName)) [[unlikely]]
			{
				parserContext.AddErrorString(std::format(LR"(ERROR: The attribute "{}" is not recognized!)", Util::General::StringToWString(attributeName)));
				return false;
			}

			if (!parserContext.Expect(":")) [[unlikely]]
			{
				parserContext.AddErrorString(L"SYNTAX ERROR: A colon (':') is expected between an attribute's name and its value!");
				return false;
			}

			// The attributeValueVerificationMap will check if the provided attribute value is
			// valid for this attribute. If so, then the respective entry in the map returns true and
			// sets mBCAInfoResolver to the relevant function pointer. Otherwise, it adds an error to
			// the BCAInfoParserContext and returns false.

			return attributeValueVerificationMap.at(attributeName)(parserContext, mBCAInfoResolver);
		}

		void AttributeParser::ResolveBCAInfo(BCAInfo& bcaInfo) const
		{
			assert(mBCAInfoResolver != nullptr && "ERROR: An AttributeParser instance was never given a valid function to use to resolve BCAInfo instances!");
			mBCAInfoResolver(bcaInfo);
		}
	}
}