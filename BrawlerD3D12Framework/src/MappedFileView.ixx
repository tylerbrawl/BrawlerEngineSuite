module;
#include <span>
#include <filesystem>
#include <memory>
#include <cassert>
#include "DxDef.h"

export module Brawler.MappedFileView;
import Brawler.FileAccessMode;
import Brawler.Win32.SafeHandle;
import Util.General;
import Util.Win32;

namespace Brawler
{
	using MappedAddress_T = LPVOID;

	struct MappedAddressDeleter
	{
		void operator()(MappedAddress_T mappedAddress) const
		{
			if (mappedAddress != nullptr)
			{
				const bool unmapResult = UnmapViewOfFileEx(mappedAddress, 0);
				assert(unmapResult && "ERROR: UnmapViewOfFileEx() failed to unmap an address!");
			}
		}
	};

	using SafeAddressMapping = std::unique_ptr<std::remove_pointer_t<MappedAddress_T>, MappedAddressDeleter>;

	template <FileAccessMode AccessMode>
	concept IsValidAccessMode = (AccessMode != FileAccessMode::COUNT_OR_ERROR);
}

export namespace Brawler
{
	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	class MappedFileView
	{
	public:
		struct ViewParams
		{
			std::uint64_t FileOffsetInBytes;
			std::uint64_t ViewSizeInBytes;
		};

	public:
		MappedFileView() = default;
		explicit MappedFileView(const std::filesystem::path& filePath);
		MappedFileView(const std::filesystem::path& filePath, const ViewParams& params);

		MappedFileView(const MappedFileView& rhs) = delete;
		MappedFileView& operator=(const MappedFileView& rhs) = delete;

		MappedFileView(MappedFileView&& rhs) noexcept = default;
		MappedFileView& operator=(MappedFileView&& rhs) noexcept = default;

		std::span<std::byte> GetMappedData() requires (AccessMode == FileAccessMode::READ_WRITE);
		std::span<const std::byte> GetMappedData() const;

		bool IsValidView() const;

	private:
		void CreateFileMappingObject(const std::filesystem::path& filePath, const ViewParams& params);
		void MapFileView(const ViewParams& params);

	private:
		Win32::SafeHandle mHFileMappingObject;
		SafeAddressMapping mMapping;
		std::span<std::byte> mMappedSpan;
	};
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	std::uint32_t GetAllocationGranularity();
}

namespace Brawler
{
	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	MappedFileView<AccessMode>::MappedFileView(const std::filesystem::path& filePath) :
		mHFileMappingObject(nullptr),
		mMapping(nullptr),
		mMappedSpan()
	{
		std::error_code errorCode{};
		const std::size_t fileSizeInBytes = std::filesystem::file_size(filePath, errorCode);

		Util::General::CheckErrorCode(errorCode);

		const ViewParams params{
			.FileOffsetInBytes = 0,
			.ViewSizeInBytes = fileSizeInBytes
		};

		CreateFileMappingObject(filePath, params);
		MapFileView(params);
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	MappedFileView<AccessMode>::MappedFileView(const std::filesystem::path& filePath, const ViewParams& params) :
		mHFileMappingObject(nullptr),
		mMapping(nullptr),
		mMappedSpan()
	{
		CreateFileMappingObject(filePath, params);
		MapFileView(params);
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	std::span<std::byte> MappedFileView<AccessMode>::GetMappedData() requires (AccessMode == FileAccessMode::READ_WRITE)
	{
		return mMappedSpan;
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	std::span<const std::byte> MappedFileView<AccessMode>::GetMappedData() const
	{
		return mMappedSpan;
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	bool MappedFileView<AccessMode>::IsValidView() const
	{
		return (Util::Win32::IsHandleValid(mHFileMappingObject) && mMapping != nullptr);
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	void MappedFileView<AccessMode>::CreateFileMappingObject(const std::filesystem::path& filePath, const ViewParams& params)
	{
		static constexpr std::uint32_t DW_DESIRED_ACCESS = (AccessMode == FileAccessMode::READ_ONLY ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE);
		
		Win32::SafeHandle hFile{ CreateFile(
			filePath.c_str(),
			DW_DESIRED_ACCESS,

			// Allow only reading for now. According to the MSDN, this prevents other accesses
			// until the HANDLE to the created file is destroyed. This will happen once we exit
			// this function. In theory, then, we should still be able to open HANDLEs with
			// write access after leaving this function.
			FILE_SHARE_READ,

			nullptr,

			// Only allow opening existing files, regardless of what AccessMode is set to. If
			// the user wants to create a new file, they can call the constructor of
			// std::ofstream with the ios flag std::ios::out; this will destroy the contents
			// of the original file and create a new one if it does not already exist. For
			// more information, refer to https://en.cppreference.com/w/cpp/io/basic_filebuf/open.
			OPEN_EXISTING,

			// We expect memory-mapped I/O access to be largely sequential. If this needs to
			// change in the future, then we can add the caching behavior as a parameter to
			// the constructor of MappedFileView.
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,

			nullptr
		) };

		if (hFile.get() == INVALID_HANDLE_VALUE) [[unlikely]]
			Util::General::CheckHRESULT(HRESULT_FROM_WIN32(GetLastError()));

		static constexpr std::uint32_t FL_PROTECT = (AccessMode == FileAccessMode::READ_ONLY ? PAGE_READONLY : PAGE_READWRITE);

		if constexpr (Util::General::IsDebugModeEnabled()) 
		{
			// According to the MSDN, file mapping fails if the size of the file is 0.
			std::error_code errorCode{};

			const std::size_t fileSize = std::filesystem::file_size(filePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			assert(fileSize != 0 && "ERROR: File mappings cannot be created if the size of the file is 0 bytes! (To efficiently change the size of a file, use std::filesystem::resize_file().)");
		}

		mHFileMappingObject.reset(CreateFileMapping(
			hFile.get(),
			nullptr,
			FL_PROTECT,
			0,
			0,
			nullptr
		));

		if (mHFileMappingObject == nullptr) [[unlikely]]
			Util::General::CheckHRESULT(HRESULT_FROM_WIN32(GetLastError()));
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	void MappedFileView<AccessMode>::MapFileView(const ViewParams& params)
	{
		static constexpr std::uint32_t DW_DESIRED_ACCESS = (AccessMode == FileAccessMode::READ_ONLY ? FILE_MAP_READ : FILE_MAP_WRITE);

		// We want the returned std::span to reflect the user's desired view of the file. However,
		// the Win32 API requires that our offset be a multiple of the system allocation granularity.
		// Therefore, we need to offset our start address by moving it backwards. To make sure that
		// we are reading all of the data, however, we also need to increase the size of the mapped
		// view.
		const std::size_t fileOffsetDelta = (params.FileOffsetInBytes % GetAllocationGranularity());

		const std::size_t adjustedFileOffset = (params.FileOffsetInBytes - fileOffsetDelta);
		const std::size_t numBytesToView = (params.ViewSizeInBytes + fileOffsetDelta);

		mMapping.reset(MapViewOfFileEx(
			mHFileMappingObject.get(),
			DW_DESIRED_ACCESS,
			(adjustedFileOffset >> 32),
			(adjustedFileOffset & 0xFFFFFFFF),
			numBytesToView,
			nullptr
		));

		if (mMapping == nullptr) [[unlikely]]
			Util::General::CheckHRESULT(HRESULT_FROM_WIN32(GetLastError()));

		// Adjust the mapped std::span to be equivalent to the user's desired view.
		mMappedSpan = std::span<std::byte>{ (reinterpret_cast<std::byte*>(mMapping.get()) + fileOffsetDelta), params.ViewSizeInBytes };
	}
}