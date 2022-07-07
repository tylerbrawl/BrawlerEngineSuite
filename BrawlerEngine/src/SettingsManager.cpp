module;
#include <type_traits>
#include <array>
#include <variant>
#include <atomic>
#include <cassert>
#include <string>
#include <algorithm>
#include <filesystem>
#include <mutex>

module Brawler.SettingsManager;
import Brawler.SettingID;
import Brawler.IMPL.SettingsDef;
import Util.General;
import Brawler.INIManager;
import Brawler.Functional;
import Util.Win32;
import Brawler.Win32.FolderPath;
import Brawler.Manifest;
import Brawler.NZStringView;

namespace
{
	// Use separate configuration files between debug and release builds.
#ifdef _DEBUG
	static constexpr Brawler::NZWStringView CONFIG_FILE_NAME{ L"Config_Debug.ini" };
#else
	static constexpr Brawler::NZWStringView CONFIG_FILE_NAME{ L"Config.ini" };
#endif  // _DEBUG
}

namespace Brawler
{
	SettingsManager::SettingsManager() :
		mSettingMap(),
		mCritSection()
	{
		Initialize();
	}

	SettingsManager::~SettingsManager()
	{
		SaveConfigurationFile();
	}

	void SettingsManager::Initialize()
	{
		// First, initialize all of the values to their defaults.
		RestoreDefaultSettings();

		// Then, overwrite the defaults with user-defined values found in the configuration
		// file.
		LoadConfigurationFile();
	}

	SettingsManager& SettingsManager::GetInstance()
	{
		static SettingsManager instance{};
		return instance;
	}

	void SettingsManager::SaveConfigurationFile() const
	{
		INIManager iniManager{};
		std::scoped_lock<std::mutex> lock{ mCritSection };

		const auto workerLambda = [this, &iniManager]<Brawler::SettingID CurrID>(this const auto& self)
		{
			if constexpr (CurrID != Brawler::SettingID::COUNT_OR_ERROR)
			{
				using CurrentSettingDefinition = Brawler::IMPL::SettingDefinition<CurrID>;
				
				iniManager.AddConfigOption<CurrentSettingDefinition::Type>(
					std::string{ Brawler::GetSettingHeaderString<CurrentSettingDefinition::HEADER>().C_Str()},
					std::string{ Brawler::GetSettingIDString<CurrID>().C_Str() },
					std::get<std::to_underlying(CurrID)>(mSettingMap[std::to_underlying(CurrID)])
				);

				constexpr Brawler::SettingID NEXT_ID = static_cast<Brawler::SettingID>(std::to_underlying(CurrID) + 1);
				self.operator()<NEXT_ID>();
			}
		};
		workerLambda.operator()<static_cast<Brawler::SettingID>(0)>();

		/* First, try to save the options in %LOCALAPPDATA%\[Application Name]\Config.ini. */
		std::optional<std::filesystem::path> currPath{ Util::Win32::GetKnownFolderPath(Win32::FolderPath::LOCAL_APP_DATA) };
		if (currPath) [[likely]]
		{
			iniManager.SaveToFile(std::filesystem::path{ std::move(*currPath) } / Brawler::Manifest::APPLICATION_NAME.C_Str() / CONFIG_FILE_NAME.C_Str());
			return;
		}

		/* If SHGetKnownFolderPath() failed, then try to save it in %USERPROFILE%\Saved Games\[Application Name]\Config.ini. */
		currPath = std::move(Util::Win32::GetKnownFolderPath(Win32::FolderPath::SAVED_GAMES));
		if (currPath) [[likely]]
		{
			iniManager.SaveToFile(std::filesystem::path{ std::move(*currPath) } / Brawler::Manifest::APPLICATION_NAME.C_Str() / CONFIG_FILE_NAME.C_Str());
			return;
		}

		// If it failed again, then just save it in the current working directory.
		iniManager.SaveToFile(std::filesystem::current_path() / CONFIG_FILE_NAME.C_Str());
	}

	bool SettingsManager::LoadConfigurationFile()
	{	
		INIManager iniManager{};
		std::scoped_lock<std::mutex> lock{ mCritSection };

		const auto workerLambda = [this, &iniManager]<Brawler::SettingID CurrID>(this const auto& self)
		{
			if constexpr (CurrID != Brawler::SettingID::COUNT_OR_ERROR)
			{
				using CurrentSettingDefinition = Brawler::IMPL::SettingDefinition<CurrID>;

				std::optional<CurrentSettingDefinition::Type> settingValue{ iniManager.GetConfigOption<CurrentSettingDefinition::Type>(
					std::string{ Brawler::GetSettingHeaderString<CurrentSettingDefinition::HEADER>() },
					std::string{ Brawler::GetSettingIDString<CurrID>() }
				) };

				if (settingValue.has_value()) [[likely]]
					mSettingMap[std::to_underlying(CurrID)].emplace<std::to_underlying(CurrID)>(std::move(*settingValue));

				constexpr Brawler::SettingID NEXT_ID = static_cast<Brawler::SettingID>(std::to_underlying(CurrID) + 1);
				self.operator()<NEXT_ID>();
			}
		};
		
		/* First, try to load the options from %LOCALAPPDATA%\[Application Name]\Config.ini. */
		std::optional<std::wstring> currPath{ Util::Win32::GetKnownFolderPath(Win32::FolderPath::LOCAL_APP_DATA) };
		if (currPath && iniManager.LoadFromFile(std::filesystem::path{std::move(*currPath)} / Brawler::Manifest::APPLICATION_NAME.C_Str() / CONFIG_FILE_NAME.C_Str()))
		{
			workerLambda.operator()<static_cast<Brawler::SettingID>(0)>();
			return true;
		}
		
		/* If SHGetKnownFolderPath() failed, then try to load it from %USERPROFILE%\Saved Games\[Application Name]\Config.ini. */
		currPath = std::move(Util::Win32::GetKnownFolderPath(Win32::FolderPath::SAVED_GAMES));
		if (currPath && iniManager.LoadFromFile(std::filesystem::path{ std::move(*currPath) } / Brawler::Manifest::APPLICATION_NAME.C_Str() / CONFIG_FILE_NAME.C_Str()))
		{
			workerLambda.operator()<static_cast<Brawler::SettingID>(0)>();
			return true;
		}

		// If it failed again, then try one last time from the current working directory.
		if (iniManager.LoadFromFile(std::filesystem::current_path() / CONFIG_FILE_NAME.C_Str()))
		{
			workerLambda.operator()<static_cast<Brawler::SettingID>(0)>();
			return true;
		}

		return false;
	}

	void SettingsManager::RestoreDefaultSettings()
	{
		// We need to lock it *before* creating the lambda function, because it captures the this pointer.
		std::scoped_lock<std::mutex> lock{ mCritSection };

		const auto workerLambda = [this] <Brawler::SettingID CurrID> (this const auto& self)
		{
			if constexpr (CurrID != Brawler::SettingID::COUNT_OR_ERROR)
			{
				using CurrentSettingDefinition = Brawler::IMPL::SettingDefinition<CurrID>;
				
				mSettingMap[std::to_underlying(CurrID)].emplace<std::to_underlying(CurrID)>(CurrentSettingDefinition::DEFAULT_VALUE);

				constexpr Brawler::SettingID NEXT_ID = static_cast<Brawler::SettingID>(std::to_underlying(CurrID) + 1);
				self.operator()<NEXT_ID>();
			}
		};
		workerLambda.operator()<static_cast<Brawler::SettingID>(0)>();
	}
}