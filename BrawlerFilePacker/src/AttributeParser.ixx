module;
#include <compare>  // What the hell? Why do I need this here?

export module Brawler.BCAInfoParsing.AttributeParser;
import Brawler.BCAInfoParsing.BCAInfoParserContext;
import Brawler.BCAInfo;

namespace Brawler
{
	namespace BCAInfoParsing
	{
		template <typename RetType, typename... Args>
		using FunctionPtr = RetType(*)(Args...);
	}
}

export namespace Brawler
{
	namespace BCAInfoParsing
	{
		class AttributeParser
		{
		public:
			AttributeParser() = default;

			AttributeParser(const AttributeParser& rhs) = delete;
			AttributeParser& operator=(const AttributeParser& rhs) = delete;

			AttributeParser(AttributeParser&& rhs) noexcept = default;
			AttributeParser& operator=(AttributeParser&& rhs) noexcept = default;

			bool ParseAttribute(BCAInfoParserContext& parserContext);

			void ResolveBCAInfo(BCAInfo& bcaInfo) const;

		private:
			FunctionPtr<void, BCAInfo&> mBCAInfoResolver;
		};
	}
}