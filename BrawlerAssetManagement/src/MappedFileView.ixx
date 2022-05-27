module;
#include <span>
#include <filesystem>
#include <memory>
#include <cassert>
#include <DxDef.h>

export module Brawler.MappedFileView;
import Brawler.FileAccessMode;
import Brawler.Win32.SafeHandle;
import Brawler.FileMapper;

namespace Brawler
{
	using MappedAddress_T = LPVOID;
	
	static constexpr auto UNMAP_ADDRESS_LAMBDA = [] (MappedAddress_T mappedAddress)
	{
		if (mappedAddress != nullptr)
		{
			const bool unmapResult = UnmapViewOfFileEx(mappedAddress, 0);
			assert(unmapResult && "ERROR: UnmapViewOfFileEx() failed to unmap an address!");
		}
	};

	using SafeAddressMapping = std::unique_ptr<std::remove_pointer_t<MappedAddress_T>, decltype(UNMAP_ADDRESS_LAMBDA)>;

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
		MappedFileView(const std::filesystem::path& filePath, const ViewParams& params);
		MappedFileView(const HANDLE hFileMappingObject, const ViewParams& params);

		~MappedFileView() = default;

		MappedFileView(const MappedFileView& rhs) = delete;
		MappedFileView& operator=(const MappedFileView& rhs) = delete;

		MappedFileView(MappedFileView&& rhs) noexcept = default;
		MappedFileView& operator=(MappedFileView&& rhs) noexcept = default;

		std::span<std::byte> GetMappedData() requires (AccessMode == FileAccessMode::READ_WRITE);
		std::span<const std::byte> GetMappedData() const;

		bool IsValidView() const;

	private:
		void MapFileView(const HANDLE hFileMappingObject, const ViewParams& params);

	private:
		SafeAddressMapping mMapping;
		std::span<std::byte> mMappedSpan;
	};
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	static const std::uint32_t allocationGranularity = [] ()
	{
		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);

		return sysInfo.dwAllocationGranularity;
	}();
}

namespace Brawler
{
	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	MappedFileView<AccessMode>::MappedFileView(const std::filesystem::path& filePath, const ViewParams& params) :
		mMapping(nullptr),
		mMappedSpan()
	{
		MapFileView(FileMapper::GetInstance().GetFileMappingObject(filePath), params);
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	MappedFileView<AccessMode>::MappedFileView(const HANDLE hFileMappingObject, const ViewParams& params) :
		mMapping(nullptr),
		mMappedSpan()
	{
		MapFileView(hFileMappingObject, params);
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
		return (mMapping != nullptr);
	}

	template <FileAccessMode AccessMode>
		requires IsValidAccessMode<AccessMode>
	void MappedFileView<AccessMode>::MapFileView(const HANDLE hFileMappingObject, const ViewParams& params)
	{
		assert(hFileMappingObject != nullptr && hFileMappingObject != INVALID_HANDLE_VALUE && "ERROR: An invalid HANDLE value was specified when creating a MappedFileView!");

		static constexpr std::uint32_t DESIRED_ACCESS = []<FileAccessMode AccessMode>()
		{
			if constexpr (AccessMode == FileAccessMode::READ_ONLY)
				return FILE_MAP_READ;
			else
				return FILE_MAP_WRITE;
		}.operator()<AccessMode>();

		// We want the returned std::span to reflect the user's desired view of the file. However,
		// the Win32 API requires that our offset be a multiple of the system allocation granularity.
		// Therefore, we need to offset our start address by moving it backwards. To make sure that
		// we are reading all of the data, however, we also need to increase the size of the mapped
		// view.
		const std::size_t fileOffsetDelta = (params.FileOffsetInBytes % allocationGranularity);

		const std::size_t adjustedFileOffset = (params.FileOffsetInBytes - fileOffsetDelta);
		const std::size_t numBytesToView = (params.ViewSizeInBytes + fileOffsetDelta);

		mMapping.reset(MapViewOfFileEx(
			hFileMappingObject,
			DESIRED_ACCESS,
			(adjustedFileOffset >> 32),
			(adjustedFileOffset & 0xFFFFFFFF),
			numBytesToView,
			nullptr
		));

		// Adjust the mapped std::span to be equivalent to the user's desired view.
		mMappedSpan = std::span<std::byte>{ (reinterpret_cast<std::byte*>(mMapping.get()) + fileOffsetDelta), params.ViewSizeInBytes };
	}
}