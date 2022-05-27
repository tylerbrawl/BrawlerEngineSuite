module;
#include <unordered_map>
#include <filesystem>
#include <mutex>
#include <DxDef.h>

export module Brawler.FileMapper;
import Brawler.Win32.SafeHandle;

export namespace Brawler
{
	class FileMapper final
	{
	private:
		FileMapper() = default;

	public:
		~FileMapper() = default;

		FileMapper(const FileMapper& rhs) = delete;
		FileMapper& operator=(const FileMapper& rhs) = delete;

		FileMapper(FileMapper&& rhs) noexcept = delete;
		FileMapper& operator=(FileMapper&& rhs) noexcept = delete;

		static FileMapper& GetInstance();

		HANDLE GetFileMappingObject(const std::filesystem::path& filePath);

	private:
		std::unordered_map<std::filesystem::path, Win32::SafeHandle> mFileMappingObjectMap;
		mutable std::mutex mCritSection;
	};
}