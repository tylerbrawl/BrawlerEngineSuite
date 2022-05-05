module;
#include <string>
#include <filesystem>
#include <map>
#include <optional>

export module Brawler.INIManager;

export namespace Brawler
{
	class INIManager
	{
	private:
		using SettingGroup = std::map<std::string, std::string>;

	public:
		INIManager();

		template <typename T>
		void AddConfigOption(std::string&& headerName, std::string&& settingName, const T& settingValue);

		template <>
		void AddConfigOption(std::string&& headerName, std::string&& settingName, const bool& settingValue);

		template <typename T>
		std::optional<T> GetConfigOption(const std::string& headerName, const std::string& settingName) const;

		template <typename T>
			requires std::is_integral_v<T> && std::is_signed_v<T>
		std::optional<T> GetConfigOption(const std::string& headerName, const std::string& settingName) const;

		template <typename T>
			requires std::is_integral_v<T> && std::is_unsigned_v<T>
		std::optional<T> GetConfigOption(const std::string& headerName, const std::string& settingName) const;

		template <typename T>
			requires std::is_floating_point_v<T>
		std::optional<T> GetConfigOption(const std::string& headerName, const std::string& settingName) const;

		template <>
		std::optional<bool> GetConfigOption(const std::string& headerName, const std::string& settingName) const;

		SettingGroup& operator[](const std::string& headerName);
		const SettingGroup& operator[](const std::string& headerName) const;

		void SaveToFile(const std::filesystem::path& filePath) const;
		bool LoadFromFile(const std::filesystem::path& filePath);

	private:
		std::map<std::string, SettingGroup> mSettingHeaderMap;
	};
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	void INIManager::AddConfigOption(std::string&& headerName, std::string&& settingName, const T& settingValue)
	{
		mSettingHeaderMap[std::move(headerName)][std::move(settingName)] = std::to_string(settingValue);
	}

	template <>
	void INIManager::AddConfigOption(std::string&& headerName, std::string&& settingName, const bool& settingValue)
	{
		mSettingHeaderMap[std::move(headerName)][std::move(settingName)] = (settingValue ? "TRUE" : "FALSE");
	}

	template <typename T>
	std::optional<T> INIManager::GetConfigOption(const std::string& headerName, const std::string& settingName) const
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: An attempt was made to get a configuration option as an undefined type!");
	}

	template <typename T>
		requires std::is_integral_v<T> && std::is_signed_v<T>
	std::optional<T> INIManager::GetConfigOption(const std::string& headerName, const std::string& settingName) const
	{
		if (!mSettingHeaderMap.contains(headerName) || !mSettingHeaderMap.at(headerName).contains(settingName))
			return std::optional<T>{};

		return std::optional<T>{ static_cast<T>(std::stoll(mSettingHeaderMap.at(headerName).at(settingName))) };
	}

	template <typename T>
		requires std::is_integral_v<T> && std::is_unsigned_v<T>
	std::optional<T> INIManager::GetConfigOption(const std::string& headerName, const std::string& settingName) const
	{
		if (!mSettingHeaderMap.contains(headerName) || !mSettingHeaderMap.at(headerName).contains(settingName))
			return std::optional<T>{};

		// If we find that the value has a negative sign, but we were trying to get an *unsigned*
		// integer, then we reject the stored value.
		if (mSettingHeaderMap.at(headerName).at(settingName)[0] == '-')
			return std::optional<T>{};

		return std::optional<T>{ static_cast<T>(std::stoull(mSettingHeaderMap.at(headerName).at(settingName))) };
	}

	template <typename T>
		requires std::is_floating_point_v<T>
	std::optional<T> INIManager::GetConfigOption(const std::string& headerName, const std::string& settingName) const
	{
		if (!mSettingHeaderMap.contains(headerName) || !mSettingHeaderMap.at(headerName).contains(settingName))
			return std::optional<T>{};

		return std::optional<T>{ static_cast<T>(std::stof(mSettingHeaderMap.at(headerName).at(settingName))) };
	}

	template <>
	std::optional<bool> INIManager::GetConfigOption(const std::string& headerName, const std::string& settingName) const
	{
		if (!mSettingHeaderMap.contains(headerName) || !mSettingHeaderMap.at(headerName).contains(settingName))
			return std::optional<bool>{};

		// Transform the string so that it consists of uppercase letters.
		std::string upperCopy{ mSettingHeaderMap.at(headerName).at(settingName) };
		for (auto& c : upperCopy)
			c = static_cast<std::uint8_t>(std::toupper(c));

		return std::optional<bool>{ upperCopy == "TRUE" };
	}
}