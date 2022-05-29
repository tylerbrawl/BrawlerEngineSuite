module;
#include <compare>
#include <filesystem>
#include <string_view>
#include <vector>
#include <span>
#include <optional>

export module Brawler.BCAInfoParsing.BCAInfoParserContext;

export namespace Brawler
{
	namespace BCAInfoParsing
	{
		class BCAInfoParserContext
		{
		public:
			BCAInfoParserContext() = default;
			explicit BCAInfoParserContext(std::filesystem::path&& bcaInfoFilePath);

			BCAInfoParserContext(const BCAInfoParserContext& rhs) = delete;
			BCAInfoParserContext& operator=(const BCAInfoParserContext& rhs) = delete;

			BCAInfoParserContext(BCAInfoParserContext&& rhs) noexcept = default;
			BCAInfoParserContext& operator=(BCAInfoParserContext&& rhs) noexcept = default;

			bool Expect(const std::string_view expectedStr);

			/// <summary>
			/// Moves to the next character without skipping whitespace. If the BCAInfoParserContext
			/// is already at the end of the file, then this function does nothing.
			/// </summary>
			void Consume();

			std::optional<std::uint8_t> Peek();
			std::optional<std::string_view> Peek(const std::size_t numChars);

			void ToggleWhiteSpaceSkipping();

			bool IsFinished();

			const std::filesystem::path& GetBCAInfoFilePath() const;

			void AddErrorString(const std::wstring_view errorMsg);

			bool HasErrors() const;
			std::span<const std::wstring> GetErrorStringSpan() const;

		private:
			void InitializeFileData();

			void SkipWhiteSpace();

		private:
			std::filesystem::path mBCAInfoFilePath;
			std::vector<std::wstring> mErrorMsgArr;
			std::vector<std::uint8_t> mFileByteArr;
			std::size_t mCurrIndex;
			std::size_t mCurrLineNumber;
			bool mSkipWhiteSpace;
		};

		class ScopedWhiteSpaceSkipToggle
		{
		public:
			ScopedWhiteSpaceSkipToggle(BCAInfoParserContext& parserContext);

			~ScopedWhiteSpaceSkipToggle();

			ScopedWhiteSpaceSkipToggle(const ScopedWhiteSpaceSkipToggle& rhs) = delete;
			ScopedWhiteSpaceSkipToggle& operator=(const ScopedWhiteSpaceSkipToggle& rhs) = delete;

			ScopedWhiteSpaceSkipToggle(ScopedWhiteSpaceSkipToggle&& rhs) noexcept = delete;
			ScopedWhiteSpaceSkipToggle& operator=(ScopedWhiteSpaceSkipToggle&& rhs) noexcept = delete;

		private:
			BCAInfoParserContext* mParserContextPtr;
		};
	}
}