module;
#include <map>
#include <string>
#include <filesystem>
#include <fstream>
#include <cassert>

module Brawler.INIManager;

namespace
{
	// INI Context-Free Grammar:
	//
	// header_list -> header_settings header_list | header_settings
	//
	// header_settings -> header settings_list
	//
	// header -> [ . id . ]
	//
	// settings_list -> setting settings_list | setting | [Empty]
	//
	// setting -> id . = . assignment_value
	//
	// assignment_value -> float | integer | TRUE | FALSE

	class INIParser
	{
	private:
		using SettingGroup = std::map<std::string, std::string>;

	public:
		explicit INIParser(const std::filesystem::path& filePath) :
			mInputString(),
			mCurrIndex(0),
			mSettingHeaderMap()
		{
			if (!std::filesystem::exists(filePath))
				return;

			std::ifstream inputStream{ filePath };
			std::stringstream strStream{};
			std::string tmpStr{};

			while (std::getline(inputStream, tmpStr))
				strStream << tmpStr;

			mInputString = std::move(strStream.str());
			ParseHeaderList();
		}

		std::map<std::string, SettingGroup>&& GetSettingHeaderMap()
		{
			return std::move(mSettingHeaderMap);
		}

	private:
		void SkipWhitespace()
		{
			while (mCurrIndex < mInputString.size() && std::isspace(mInputString[mCurrIndex]))
				++mCurrIndex;
		}

		bool Expect(const std::string_view expectedStr, bool ignoreCase = false)
		{
			SkipWhitespace();
			
			if ((mCurrIndex + expectedStr.size() - 1) >= mInputString.size())
				return false;

			std::string_view inputView{ &(mInputString[mCurrIndex]), expectedStr.size() };

			if (ignoreCase)
			{
				std::string inputLowerCopy{ inputView };
				for (auto& c : inputLowerCopy)
					c = static_cast<std::uint8_t>(std::tolower(c));

				std::string expectedLowerCopy{ expectedStr };
				for (auto& c : expectedLowerCopy)
					c = static_cast<std::uint8_t>(std::tolower(c));

				if (inputLowerCopy != expectedLowerCopy)
					return false;
			}
			else if (inputView != expectedStr)
				return false;

			mCurrIndex += expectedStr.size();
			return true;
		}

		bool ParseID(std::string& id)
		{
			SkipWhitespace();

			std::string tmpStr{};
			std::size_t workingIndex = mCurrIndex;

			while (workingIndex < mInputString.size() && std::isalpha(mInputString[workingIndex]))
			{
				tmpStr += mInputString[workingIndex++];
			}

			if (!Expect(tmpStr))
				return false;

			id = std::move(tmpStr);
			return true;
		}

		bool ParseHeader(std::string& headerName)
		{
			if (!Expect("["))
				return false;

			std::string id{};
			if (!ParseID(id))
				return false;

			if (!Expect("]"))
				return false;

			headerName = std::move(id);
			return true;
		}

		bool ParseFloat(std::string& floatValue)
		{
			SkipWhitespace();

			std::string tmpStr{};
			std::size_t workingIndex = mCurrIndex;

			if (workingIndex >= mInputString.size())
				return false;

			if (mInputString[workingIndex] == '-')
			{
				tmpStr += '-';
				++workingIndex;

				if (workingIndex >= mInputString.size())
					return false;
			}

			bool numAdded = false;
			while (workingIndex < mInputString.size() && mInputString[workingIndex] != '.')
			{
				if (std::isdigit(mInputString[workingIndex]))
				{
					numAdded = true;
					tmpStr += mInputString[workingIndex++];
				}
				else
					break;
			}

			if (!numAdded || workingIndex >= mInputString.size() || mInputString[workingIndex] != '.')
				return false;

			tmpStr += '.';
			++workingIndex;

			numAdded = false;
			while (workingIndex < mInputString.size() && std::isdigit(mInputString[workingIndex]))
			{
				numAdded = true;
				tmpStr += mInputString[workingIndex++];
			}

			if (!numAdded)
				return false;

			Expect(tmpStr);
			floatValue = std::move(tmpStr);

			return true;
		}

		bool ParseInteger(std::string& assignmentValue)
		{
			SkipWhitespace();

			std::string tmpStr{};
			std::size_t workingIndex = mCurrIndex;

			if (workingIndex >= mInputString.size())
				return false;

			if (mInputString[workingIndex] == '-')
			{
				tmpStr += '-';
				++workingIndex;

				if (workingIndex >= mInputString.size())
					return false;
			}

			bool numAdded = false;
			while (workingIndex < mInputString.size() && std::isdigit(mInputString[workingIndex]))
			{
				tmpStr += mInputString[workingIndex++];
				numAdded = true;
			}
			
			if (!numAdded)
				return false;

			Expect(tmpStr);
			assignmentValue = std::move(tmpStr);

			return true;
		}

		bool ParseAssignmentValue(std::string& assignmentValue)
		{
			if (Expect("TRUE", true))
			{
				assignmentValue = "TRUE";
				return true;
			}

			if (Expect("FALSE", true))
			{
				assignmentValue = "FALSE";
				return true;
			}

			if (ParseFloat(assignmentValue))
				return true;

			return ParseInteger(assignmentValue);
		}

		bool ParseSetting(const std::string& headerName)
		{
			std::string settingName{};
			if (!ParseID(settingName))
				return false;

			if (!Expect("="))
				return false;

			std::string settingValue{};
			if (!ParseAssignmentValue(settingValue))
				return false;

			mSettingHeaderMap[headerName][std::move(settingName)] = std::move(settingValue);
			return true;
		}

		void ParseSettingsList(const std::string& headerName)
		{
			while (ParseSetting(headerName));
		}

		bool ParseHeaderSettings()
		{
			std::string headerName{};
			if (!ParseHeader(headerName))
				return false;

			ParseSettingsList(headerName);

			return true;
		}

		void ParseHeaderList()
		{
			while (ParseHeaderSettings());
		}

	private:
		std::string mInputString;
		std::size_t mCurrIndex;
		std::map<std::string, SettingGroup> mSettingHeaderMap;
	};
}

namespace Brawler
{
	INIManager::INIManager() :
		mSettingHeaderMap()
	{}

	INIManager::SettingGroup& INIManager::operator[](const std::string& headerName)
	{
		return mSettingHeaderMap[headerName];
	}

	const INIManager::SettingGroup& INIManager::operator[](const std::string& headerName) const
	{
		assert(mSettingHeaderMap.contains(headerName));
		return mSettingHeaderMap.at(headerName);
	}

	void INIManager::SaveToFile(const std::filesystem::path& filePath) const
	{
		// The documentation on cppreference states that if a file does not exist, then 
		// std::ofstream's constructor will create it for you.
		//
		// What it does NOT state is that if the parent path of this file (that is, the
		// path without the file name) does not exist, then it will fail to create and
		// write anything.
		if (!std::filesystem::exists(filePath.parent_path()))
			std::filesystem::create_directories(filePath.parent_path());
		
		std::ofstream outputStream{ filePath };
		bool addNewLineBeforeHeader = false;

		for (const auto& headerItr : mSettingHeaderMap)
		{
			if (addNewLineBeforeHeader)
				outputStream << "\n";
			
			outputStream << "[" << headerItr.first << "]\n";

			for (const auto& settingItr : headerItr.second)
				outputStream << settingItr.first << "=" << settingItr.second << "\n";

			addNewLineBeforeHeader = true;
		}
	}

	bool INIManager::LoadFromFile(const std::filesystem::path& filePath)
	{
		INIParser parser{ filePath };
		mSettingHeaderMap = std::move(parser.GetSettingHeaderMap());

		return !(mSettingHeaderMap.empty());
	}
}