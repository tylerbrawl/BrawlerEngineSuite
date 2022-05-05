module;
#include <filesystem>
#include <cassert>
#include "Win32Def.h"

module Brawler.FileMapper;
import Brawler.MappedFileView;

namespace
{
	static const std::uint32_t MEMORY_ALLOCATION_GRANULARITY = [] ()
	{
		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);

		return sysInfo.dwAllocationGranularity;
	}();
}

namespace Brawler
{
	FileMapper::FileMapper(const std::filesystem::path& filePath, const FileAccessMode accessMode) :
		mAccessMode(accessMode),
		mHFile(nullptr),
		mHMappingObject(nullptr)
	{
		assert(std::filesystem::exists(filePath) && "ERROR: An attempt was made to create a FileMapper for a file which does not exist!");

		{
			const DWORD dwDesiredAccess = (accessMode == FileAccessMode::READ_WRITE ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ);
			
			mHFile = CreateFile(
				filePath.c_str(),
				dwDesiredAccess,
				0,
				nullptr,
				OPEN_EXISTING,
				FILE_FLAG_SEQUENTIAL_SCAN,
				nullptr
			);

			if (mHFile == INVALID_HANDLE_VALUE) [[unlikely]]
				throw std::runtime_error{ "ERROR: The attempt to open a file handle for a FileMapper failed!" };
		}
		
		{
			const DWORD flProtect = (accessMode == FileAccessMode::READ_WRITE ? PAGE_READWRITE : PAGE_READONLY);

			mHMappingObject = CreateFileMapping(
				mHFile,
				nullptr,
				flProtect,
				0,
				0,
				nullptr
			);

			if (mHMappingObject == nullptr) [[unlikely]]
				throw std::runtime_error{ "ERROR: The attempt to create a file mapping object for a FileMapper failed!" };
		}
	}

	FileMapper::~FileMapper()
	{
		CloseHandles();
	}

	FileMapper::FileMapper(FileMapper&& rhs) noexcept :
		mAccessMode(rhs.mAccessMode),
		mHFile(rhs.mHFile),
		mHMappingObject(rhs.mHMappingObject)
	{
		rhs.mHFile = nullptr;
		rhs.mHMappingObject = nullptr;
	}

	FileMapper& FileMapper::operator=(FileMapper&& rhs) noexcept
	{
		CloseHandles();

		mAccessMode = rhs.mAccessMode;

		mHFile = rhs.mHFile;
		rhs.mHFile = nullptr;

		mHMappingObject = rhs.mHMappingObject;
		rhs.mHMappingObject = nullptr;

		return *this;
	}

	MappedFileView FileMapper::CreateMappedFileView(const std::uint64_t fileOffsetInBytes, const std::uint64_t viewSizeInBytes) const
	{
		const MappedFileView::ViewParams viewParams{
			.Mapper = *this,
			.FileOffsetInBytes = fileOffsetInBytes,
			.ViewSizeInBytes = viewSizeInBytes
		};
		return MappedFileView{ viewParams };
	}

	std::uint64_t FileMapper::GetAlignedFileOffset(const std::uint64_t fileOffsetInBytes)
	{
		const std::uint32_t remainder = (fileOffsetInBytes % MEMORY_ALLOCATION_GRANULARITY);
		return (fileOffsetInBytes - remainder);
	}

	FileAccessMode FileMapper::GetFileAccessMode() const
	{
		return mAccessMode;
	}

	HANDLE FileMapper::GetMappingObjectHandle() const
	{
		return mHMappingObject;
	}

	void FileMapper::CloseHandles()
	{
		if (mHMappingObject != nullptr) [[likely]]
		{
			CloseHandle(mHMappingObject);
			mHMappingObject = nullptr;
		}

		if (mHFile != nullptr) [[likely]]
		{
			CloseHandle(mHFile);
			mHFile = nullptr;
		}
	}
}