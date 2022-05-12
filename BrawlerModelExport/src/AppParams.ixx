module;
#include <string>
#include <vector>
#include <filesystem>

export module Brawler.AppParams;

export namespace Brawler
{
	class AppParams
	{
	public:
		AppParams() = default;

		AppParams(const AppParams& rhs) = delete;
		AppParams& operator=(const AppParams& rhs) = delete;

		AppParams(AppParams&& rhs) noexcept = default;
		AppParams& operator=(AppParams&& rhs) noexcept = default;

		void SetModelName(const std::string_view modelName);
		const std::wstring_view GetModelName() const;

		void SetLODCount(const std::uint32_t numLODFiles);
		void AddLODFilePath(const std::uint32_t lodLevel, const std::string_view lodFilePath);

		std::uint32_t GetLODCount() const; 
		const std::filesystem::path& GetLODFilePath(const std::uint32_t lodLevel) const;

		void SetRootOutputDirectory(const std::string_view rootOutputDir);
		const std::filesystem::path& GetRootOutputDirectory() const;

	private:
		std::wstring mModelName;
		std::vector<std::filesystem::path> mInputLODFilePathArr;
		std::filesystem::path mRootOutputDirectory;
	};
}