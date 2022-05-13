module;
#include <string>
#include <vector>
#include <filesystem>
#include <span>

export module Brawler.LaunchParams;

export namespace Brawler
{
	class LaunchParams
	{
	public:
		LaunchParams() = default;

		LaunchParams(const LaunchParams& rhs) = delete;
		LaunchParams& operator=(const LaunchParams& rhs) = delete;

		LaunchParams(LaunchParams&& rhs) noexcept = default;
		LaunchParams& operator=(LaunchParams&& rhs) noexcept = default;

		void SetModelName(const std::string_view modelName);
		const std::wstring_view GetModelName() const;

		void SetLODCount(const std::uint32_t numLODFiles);
		void AddLODFilePath(const std::uint32_t lodLevel, const std::string_view lodFilePath);

		std::uint32_t GetLODCount() const; 
		std::span<const std::filesystem::path> GetLODFilePaths() const;
		const std::filesystem::path& GetLODFilePath(const std::uint32_t lodLevel) const;

		void SetRootOutputDirectory(const std::string_view rootOutputDir);
		const std::filesystem::path& GetRootOutputDirectory() const;

	private:
		std::wstring mModelName;
		std::vector<std::filesystem::path> mInputLODFilePathArr;
		std::filesystem::path mRootOutputDirectory;
	};
}