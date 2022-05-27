module;
#include <unordered_map>
#include <filesystem>
#include <mutex>
#include <DxDef.h>

module Brawler.FileMapper;

namespace
{
	Brawler::Win32::SafeHandle CreateFileMappingObject(const std::filesystem::path& canonicalFilePath)
	{
		// *LOCKED*
		//
		// This function is called from a locked context.

		using SafeHandle = Brawler::Win32::SafeHandle;

		SafeHandle hFile{ CreateFile(
			canonicalFilePath.c_str(),
			(GENERIC_READ | GENERIC_WRITE),
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		) };

		if (hFile == INVALID_HANDLE_VALUE) [[unlikely]]
			return nullptr;

		return SafeHandle{ CreateFileMapping(
			hFile.get(),
			nullptr,
			PAGE_READWRITE,
			0,
			0,
			nullptr
		) };
	}
}

namespace Brawler
{
	FileMapper& FileMapper::GetInstance()
	{
		static FileMapper instance{};
		return instance;
	}

	HANDLE FileMapper::GetFileMappingObject(const std::filesystem::path& filePath)
	{
		// *WARNING*: std::hash<std::filesystem::path> creates a hash from the string representation
		// of the path, and disregards the equivalence between instances. This means that two
		// std::filesystem::path instances could refer to the same file, but have different hashes
		// (e.g., std::hash("A.txt") != std::hash(".\\A.txt"))!
		//
		// To get around this, we always use the canonical name of the path.

		std::error_code errorCode{};
		const bool doesFileExist = std::filesystem::exists(filePath, errorCode);

		if (errorCode || !doesFileExist) [[unlikely]]
			return nullptr;

		const std::filesystem::path canonicalPath{ std::filesystem::canonical(filePath, errorCode) };

		if (errorCode) [[unlikely]]
			return nullptr;

		HANDLE hMappingObject = nullptr;

		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			if (mFileMappingObjectMap.contains(canonicalPath))
				return *(mFileMappingObjectMap.at(canonicalPath));

			Win32::SafeHandle hFileMappingObject{ CreateFileMappingObject(canonicalPath) };
			hMappingObject = hFileMappingObject.get();

			mFileMappingObjectMap.try_emplace(canonicalPath, std::move(hFileMappingObject));
		}

		return hMappingObject;
	}
}