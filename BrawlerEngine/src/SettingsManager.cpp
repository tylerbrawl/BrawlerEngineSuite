module;
#include <type_traits>
#include <array>
#include <variant>
#include <atomic>
#include <cassert>
#include <string>
#include <algorithm>
#include <filesystem>

module Brawler.SettingsManager;
import Brawler.SettingID;
import Brawler.IMPL.SettingsDef;
import Util.General;
import Brawler.CriticalSection;
import Brawler.INIManager;
import Brawler.FunctionConcepts;
import Util.Win32;
import Win32.FolderPath;
import Brawler.Manifest;

namespace
{
	static constexpr std::integer_sequence<std::underlying_type_t<Brawler::SettingID>,

		// Add setting IDs as they are needed here.

		Util::General::EnumCast(Brawler::SettingID::WINDOW_RESOLUTION_WIDTH),
		Util::General::EnumCast(Brawler::SettingID::WINDOW_RESOLUTION_HEIGHT),
		Util::General::EnumCast(Brawler::SettingID::RENDER_RESOLUTION_FACTOR),
		Util::General::EnumCast(Brawler::SettingID::USE_FULLSCREEN),
		Util::General::EnumCast(Brawler::SettingID::FRAME_RATE_LIMIT)

	> settingIDSequence{};

	static constexpr bool AreSettingsCorrectlyImplemented()
	{
		bool errorFound = false;
		std::underlying_type_t<Brawler::SettingID> idCounter = 0;

		auto workerLambda = [&errorFound, &idCounter] <std::underlying_type_t<Brawler::SettingID>... ID> (std::integer_sequence<std::underlying_type_t<Brawler::SettingID>, ID...> sequence)
		{
			errorFound = ((ID != idCounter++) || ...) || settingIDSequence.size() != static_cast<std::size_t>(Brawler::SettingID::COUNT_OR_ERROR);
		};
		workerLambda(settingIDSequence);

		return !errorFound;
	}

	static_assert(AreSettingsCorrectlyImplemented(),
		"ERROR: [Anonymous Namespace]::settingIDSequence (see SettingsManager.cpp) was not correctly created! It must contain an entry for ALL of the SettingIDs in the order which they appear in the Brawler::SettingID enumeration!");

	// Use separate configuration files between debug and release builds.
#ifdef _DEBUG
	static constexpr const wchar_t* CONFIG_FILE_NAME = L"Config_Debug.ini";
#else
	static constexpr const wchar_t* CONFIG_FILE_NAME = L"Config.ini";
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

		// Welcome to Modern C++, where the developers are hardcore masochists who will stop at nothing
		// to make sure that they can overcomplicate the design of their system!
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };

		auto workerLambda = [this, &iniManager]<std::underlying_type_t<Brawler::SettingID>... ID>(std::integer_sequence<std::underlying_type_t<Brawler::SettingID>, ID...>)
		{
			([&] ()
			{
				using SettingType = Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Type;

				iniManager.AddConfigOption<Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Type>(
					std::string{ Brawler::GetSettingHeaderString<Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Header>() },
					std::string{ Brawler::GetSettingIDString<static_cast<Brawler::SettingID>(ID)>() },
					std::get<SettingType>(mSettingMap.at(static_cast<Brawler::SettingID>(ID)))
				);
			}(), ...);
		};
		workerLambda(settingIDSequence);

		/* First, try to save the options in %LOCALAPPDATA%\[Application Name]\Config.ini. */
		std::optional<std::wstring> currPath{ Util::Win32::GetKnownFolderPath(Win32::FolderPath::LOCAL_APP_DATA) };
		if (currPath)
		{
			iniManager.SaveToFile(std::filesystem::path{ std::move(*currPath) } / Util::General::StringToWString(Brawler::Manifest::APPLICATION_NAME) / CONFIG_FILE_NAME);
			return;
		}

		/* If SHGetKnownFolderPath() failed, then try to save it in %USERPROFILE%\Saved Games\[Application Name]\Config.ini. */
		currPath = std::move(Util::Win32::GetKnownFolderPath(Win32::FolderPath::SAVED_GAMES));
		if (currPath)
		{
			iniManager.SaveToFile(std::filesystem::path{ std::move(*currPath) } / Util::General::StringToWString(Brawler::Manifest::APPLICATION_NAME) / CONFIG_FILE_NAME);
			return;
		}

		// If it failed again, then just save it in the current working directory.
		iniManager.SaveToFile(std::filesystem::current_path() / CONFIG_FILE_NAME);
	}

	bool SettingsManager::LoadConfigurationFile()
	{	
		INIManager iniManager{};
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };

		auto workerLambda = [this, &iniManager] <std::underlying_type_t<Brawler::SettingID>... ID> (std::integer_sequence<std::underlying_type_t<Brawler::SettingID>, ID...>)
		{
			([&]()
			{
				std::optional<Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Type> settingValue{ iniManager.GetConfigOption<Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Type>(
					std::string{Brawler::GetSettingHeaderString<Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Header>()},
					std::string{Brawler::GetSettingIDString<static_cast<Brawler::SettingID>(ID)>()}
				) };

				if (settingValue)
					mSettingMap[static_cast<Brawler::SettingID>(ID)].emplace<Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Type>(*settingValue);
			}(), ...);
		};
		
		/* First, try to load the options from %LOCALAPPDATA%\[Application Name]\Config.ini. */
		std::optional<std::wstring> currPath{ Util::Win32::GetKnownFolderPath(Win32::FolderPath::LOCAL_APP_DATA) };
		if (currPath && iniManager.LoadFromFile(std::filesystem::path{std::move(*currPath)} / Util::General::StringToWString(Brawler::Manifest::APPLICATION_NAME) / CONFIG_FILE_NAME))
		{
			workerLambda(settingIDSequence);
			return true;
		}
		
		/* If SHGetKnownFolderPath() failed, then try to load it from %USERPROFILE%\Saved Games\[Application Name]\Config.ini. */
		currPath = std::move(Util::Win32::GetKnownFolderPath(Win32::FolderPath::SAVED_GAMES));
		if (currPath && iniManager.LoadFromFile(std::filesystem::path{ std::move(*currPath) } / Util::General::StringToWString(Brawler::Manifest::APPLICATION_NAME) / CONFIG_FILE_NAME))
		{
			workerLambda(settingIDSequence);
			return true;
		}

		// If it failed again, then try one last time from the current working directory.
		if (iniManager.LoadFromFile(std::filesystem::current_path() / CONFIG_FILE_NAME))
		{
			workerLambda(settingIDSequence);
			return true;
		}

		return false;
	}

	void SettingsManager::RestoreDefaultSettings()
	{
		// We need to lock it *before* creating the lambda function, because it captures the this pointer.
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };

		auto workerLambda = [this] <std::underlying_type_t<Brawler::SettingID>... ID> (std::integer_sequence<std::underlying_type_t<Brawler::SettingID>, ID...>)
		{
			([&] ()
			{
				using SettingType = Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::Type;

				mSettingMap[static_cast<Brawler::SettingID>(ID)].emplace<SettingType>(Brawler::IMPL::SettingDefinition<static_cast<Brawler::SettingID>(ID)>::DefaultVal);
			}(), ...);
		};
		workerLambda(settingIDSequence);
	}
}