module;
#include <variant>
#include <unordered_map>
#include <cassert>

export module Brawler.SettingsManager;
import Brawler.SettingID;
import Brawler.IMPL.SettingsDef;
import Brawler.CriticalSection;

export namespace Brawler
{
	class SettingsManager
	{
	public:
		using SettingVariant = std::variant<
			std::uint32_t,
			bool,
			float
		>;

		template <Brawler::SettingID ID>
		using SettingType = Brawler::IMPL::SettingDefinition<ID>::Type;

	private:
		SettingsManager();

	public:
		~SettingsManager();

		SettingsManager(const SettingsManager& rhs) = delete;
		SettingsManager& operator=(const SettingsManager& rhs) = delete;

		SettingsManager(SettingsManager&& rhs) noexcept = default;
		SettingsManager& operator=(SettingsManager&& rhs) noexcept = default;

	private:
		void Initialize();

	public:
		static SettingsManager& GetInstance();

		template <SettingID ID>
		void SetOption(const SettingType<ID> value);

		template <SettingID ID>
		SettingType<ID> GetOption() const;

	private:
		void SaveConfigurationFile() const;

		/// <summary>
		/// This function attempts to restore the configuration options from a
		/// configuration file located at the default path.
		/// </summary>
		/// <returns>
		/// The function returns true if the options were successfully restored from
		/// the file and false otherwise.
		/// </returns>
		bool LoadConfigurationFile();

		void RestoreDefaultSettings();

	private:
		std::unordered_map<SettingID, SettingVariant> mSettingMap;
		mutable CriticalSection mCritSection;
	};
}

// ------------------------------------------------------------------------------------

namespace Brawler
{
	template <SettingID ID>
	void SettingsManager::SetOption(const SettingType<ID> value)
	{
		ScopedLock<CriticalSection> lock{ mCritSection };

		assert(mSettingMap.contains(ID) && "ERROR: The SettingsManager was not fully initialized before SettingsManager::SetOption<T>() was called!");
		std::get<SettingType<ID>>(mSettingMap.at(ID)) = value;
	}
	
	template <SettingID ID>
	SettingsManager::SettingType<ID> SettingsManager::GetOption() const
	{
		ScopedLock<CriticalSection> lock{ mCritSection };

		assert(mSettingMap.contains(ID) && "ERROR: The SettingsManager was not fully initialized before SettingsManager::GetOption<T>() was called!");
		return std::get<SettingType<ID>>(mSettingMap.at(ID));
	}
}