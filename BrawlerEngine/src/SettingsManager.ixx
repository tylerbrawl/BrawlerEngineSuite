module;
#include <tuple>
#include <array>
#include <cassert>
#include <mutex>

export module Brawler.SettingsManager;
import Brawler.SettingID;
import Brawler.IMPL.SettingsDef;

namespace Brawler
{
	template <SettingID CurrID>
	consteval auto GetSettingTuple()
	{
		constexpr SettingID NEXT_ID = static_cast<SettingID>(std::to_underlying(CurrID) + 1);

		if constexpr (NEXT_ID != SettingID::COUNT_OR_ERROR)
			return std::tuple_cat(std::tuple<typename IMPL::SettingDefinition<CurrID>::Type>{}, GetSettingTuple<NEXT_ID>());
		else
			return std::tuple<typename IMPL::SettingDefinition<CurrID>::Type>{};
	}
}

export namespace Brawler
{
	class SettingsManager final
	{
	public:
		using SettingTuple = decltype(GetSettingTuple<static_cast<SettingID>(0)>());

		template <Brawler::SettingID ID>
		using SettingType = typename Brawler::IMPL::SettingDefinition<ID>::Type;

	private:
		SettingsManager();

	public:
		~SettingsManager();

		SettingsManager(const SettingsManager& rhs) = delete;
		SettingsManager& operator=(const SettingsManager& rhs) = delete;

		SettingsManager(SettingsManager&& rhs) noexcept = delete;
		SettingsManager& operator=(SettingsManager&& rhs) noexcept = delete;

	private:
		void Initialize();

	public:
		static SettingsManager& GetInstance();

		template <SettingID ID>
		void SetOption(const SettingType<ID> value) requires (ID != SettingID::COUNT_OR_ERROR);

		template <SettingID ID>
		SettingType<ID> GetOption() const requires (ID != SettingID::COUNT_OR_ERROR);

		template <SettingID ID>
		static consteval SettingType<ID> GetDefaultValueForOption() requires (ID != SettingID::COUNT_OR_ERROR);

		template <SettingID ID>
		void ResetOption() requires (ID != SettingID::COUNT_OR_ERROR);

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
		SettingTuple mSettingTuple;
		mutable std::mutex mCritSection;
	};
}

// ------------------------------------------------------------------------------------

namespace Brawler
{
	template <SettingID ID>
	void SettingsManager::SetOption(const SettingType<ID> value) requires (ID != SettingID::COUNT_OR_ERROR)
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		std::get<std::to_underlying(ID)>(mSettingTuple) = value;
	}
	
	template <SettingID ID>
	SettingsManager::SettingType<ID> SettingsManager::GetOption() const requires (ID != SettingID::COUNT_OR_ERROR)
	{
		std::scoped_lock<std::mutex> lock{ mCritSection };

		return std::get<std::to_underlying(ID)>(mSettingTuple);
	}

	template <SettingID ID>
	consteval SettingsManager::SettingType<ID> SettingsManager::GetDefaultValueForOption() requires (ID != SettingID::COUNT_OR_ERROR)
	{
		// We don't need to acquire the lock for this, because we are not accessing any data from
		// the SettingsManager instance.
		
		constexpr SettingType<ID> DEFAULT_VALUE{ IMPL::SettingDefinition<ID>::DEFAULT_VALUE };
		return DEFAULT_VALUE;
	}

	template <SettingID ID>
	void SettingsManager::ResetOption() requires (ID != SettingID::COUNT_OR_ERROR)
	{
		constexpr SettingType<ID> DEFAULT_VALUE{ GetDefaultValueForOption<ID>() };
		SetOption<ID>(DEFAULT_VALUE);
	}
}