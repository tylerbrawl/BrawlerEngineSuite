module;
#include <compare>
#include <filesystem>
#include <string_view>
#include <vector>
#include <fstream>
#include <cassert>
#include <format>
#include <optional>

module Brawler.BCAInfoParsing.BCAInfoParserContext;
import Util.General;

namespace Brawler
{
	namespace BCAInfoParsing
	{
		BCAInfoParserContext::BCAInfoParserContext(std::filesystem::path&& bcaInfoFilePath) :
			mBCAInfoFilePath(std::move(bcaInfoFilePath)),
			mErrorMsgArr(),
			mFileByteArr(),
			mCurrIndex(0),
			mCurrLineNumber(1),
			mSkipWhiteSpace(true)
		{
			InitializeFileData();
		}

		bool BCAInfoParserContext::Expect(const std::string_view expectedStr)
		{
			if (IsFinished())
				return expectedStr.empty();

			if (mCurrIndex + expectedStr.size() > mFileByteArr.size()) [[unlikely]]
				return false;

			const std::string_view fileDataStr{ reinterpret_cast<char*>(mFileByteArr.data() + mCurrIndex), expectedStr.size() };

			if (fileDataStr != expectedStr) [[unlikely]]
				return false;

			mCurrIndex += expectedStr.size();
			return true;
		}

		void BCAInfoParserContext::Consume()
		{
			if (mCurrIndex < mFileByteArr.size())
				++mCurrIndex;
		}

		std::optional<std::uint8_t> BCAInfoParserContext::Peek()
		{
			if (IsFinished()) [[unlikely]]
				return std::optional<std::uint8_t>{};

			return std::optional<std::uint8_t>{ mFileByteArr[mCurrIndex] };
		}

		std::optional<std::string_view> BCAInfoParserContext::Peek(const std::size_t numChars)
		{
			if (mCurrIndex + numChars > mFileByteArr.size()) [[unlikely]]
				return std::optional<std::string_view>{};

			return std::optional<std::string_view>{ std::in_place, reinterpret_cast<char*>(mFileByteArr.data() + mCurrIndex), numChars };
		}

		void BCAInfoParserContext::ToggleWhiteSpaceSkipping()
		{
			mSkipWhiteSpace = !mSkipWhiteSpace;
		}

		bool BCAInfoParserContext::IsFinished()
		{
			SkipWhiteSpace();
			
			return (mCurrIndex >= mFileByteArr.size());
		}

		const std::filesystem::path& BCAInfoParserContext::GetBCAInfoFilePath() const
		{
			return mBCAInfoFilePath;
		}

		void BCAInfoParserContext::AddErrorString(const std::wstring_view errorMsg)
		{
			mErrorMsgArr.push_back(std::format(L"{} (Line Number: {}): {}", mBCAInfoFilePath.c_str(), mCurrLineNumber, errorMsg));
		}

		bool BCAInfoParserContext::HasErrors() const
		{
			return !mErrorMsgArr.empty();
		}

		std::span<const std::wstring> BCAInfoParserContext::GetErrorStringSpan() const
		{
			return std::span<const std::wstring>{ mErrorMsgArr };
		}

		void BCAInfoParserContext::InitializeFileData()
		{
			// We expect .BCAINFO files to be small enough that reading all of their data at once
			// is acceptable.
			
			assert(std::filesystem::equivalent(mBCAInfoFilePath.filename(), std::filesystem::path{ L".BCAINFO" }) && !std::filesystem::is_directory(mBCAInfoFilePath) && "ERROR: A BCAInfoParserContext was given a file path which did not correspond to a .BCAINFO file!");

			std::ifstream bcaInfoFileStream{ mBCAInfoFilePath, std::ios_base::in | std::ios_base::binary };

			std::error_code errorCode{};
			const std::size_t bcaInfoFileSize = std::filesystem::file_size(mBCAInfoFilePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The attempt to get the file size for the .BCAINFO file at {} failed with the following error: {}", mBCAInfoFilePath.string(), errorCode.message()) };

			mFileByteArr.resize(bcaInfoFileSize);
			bcaInfoFileStream.read(reinterpret_cast<char*>(mFileByteArr.data()), bcaInfoFileSize);
		}

		void BCAInfoParserContext::SkipWhiteSpace()
		{
			if (!mSkipWhiteSpace) [[unlikely]]
				return;
			
			while (mCurrIndex < mFileByteArr.size() && std::isspace(mFileByteArr[mCurrIndex]))
			{
				if (mFileByteArr[mCurrIndex++] == '\n')
					++mCurrLineNumber;
			}
		}

		ScopedWhiteSpaceSkipToggle::ScopedWhiteSpaceSkipToggle(BCAInfoParserContext& parserContext) :
			mParserContextPtr(std::addressof(parserContext))
		{
			mParserContextPtr->ToggleWhiteSpaceSkipping();
		}

		ScopedWhiteSpaceSkipToggle::~ScopedWhiteSpaceSkipToggle()
		{
			mParserContextPtr->ToggleWhiteSpaceSkipping();
		}
	}
}