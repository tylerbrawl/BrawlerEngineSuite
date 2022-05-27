module;
#include <span>
#include <cassert>
#include "Win32Def.h"

module Brawler.MappedFileView;
import Brawler.FileMapper;

namespace Brawler
{
	MappedFileView::MappedFileView() :
		mMappedAddress(nullptr),
		mMappedSpan()
	{}
	
	MappedFileView::MappedFileView(const ViewParams& params) :
		mMappedAddress(nullptr),
		mMappedSpan()
	{
		assert(params.Mapper.GetMappingObjectHandle() != nullptr && "ERROR: An attempt was made to create a MappedFileView using a nullptr mapping object!");

		const DWORD dwDesiredAccess = (params.Mapper.GetFileAccessMode() == FileAccessMode::READ_WRITE ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ);

		// The Win32 API requires that our file offset be a multiple of the memory
		// allocation granularity of the system.
		const std::uint64_t fileOffset = FileMapper::GetAlignedFileOffset(params.FileOffsetInBytes);

		// However, we assume that the user specifies the view size with the assumption
		// that the view starts exactly at the offset they specify. Thus, to make sure
		// that our view contains all of the necessary data, we need to increase our
		// view size by the same amount which we moved back our offset.
		const std::uint64_t fileOffsetDelta = (params.FileOffsetInBytes - fileOffset);
		const std::uint64_t viewSize =  (params.ViewSizeInBytes + fileOffsetDelta);

		mMappedAddress = reinterpret_cast<std::uint8_t*>(MapViewOfFileEx(
			params.Mapper.GetMappingObjectHandle(),
			dwDesiredAccess,
			static_cast<std::uint32_t>(fileOffset >> 32),
			static_cast<std::uint32_t>(fileOffset & 0xFFFFFFFF),
			viewSize,
			nullptr
		));

		if (mMappedAddress == nullptr) [[unlikely]]
			throw std::runtime_error{ "ERROR: MapViewOfFileEx() failed to create a file view!" };

		// We want our mapped std::span to reflect the view of the file which the user
		// asked for. So, we need to offset its starting address to be the offset which
		// they originally specified.
		mMappedSpan = std::span<std::uint8_t>{ mMappedAddress + fileOffsetDelta, params.ViewSizeInBytes };
	}

	MappedFileView::~MappedFileView()
	{
		UnmapView();
	}

	MappedFileView::MappedFileView(MappedFileView&& rhs) noexcept :
		mMappedAddress(rhs.mMappedAddress),
		mMappedSpan(rhs.mMappedSpan)
	{
		rhs.mMappedAddress = nullptr;
		rhs.mMappedSpan = std::span<std::uint8_t>{};
	}

	MappedFileView& MappedFileView::operator=(MappedFileView&& rhs) noexcept
	{
		UnmapView();

		mMappedAddress = rhs.mMappedAddress;
		rhs.mMappedAddress = nullptr;

		mMappedSpan = rhs.mMappedSpan;
		rhs.mMappedSpan = std::span<std::uint8_t>{};

		return *this;
	}

	std::span<std::uint8_t> MappedFileView::GetMappedData()
	{
		return mMappedSpan;
	}

	std::span<const std::uint8_t> MappedFileView::GetMappedData() const
	{
		return mMappedSpan;
	}

	void MappedFileView::UnmapView()
	{
		if (mMappedAddress != nullptr)
		{
			UnmapViewOfFileEx(reinterpret_cast<PVOID>(mMappedAddress), 0);
			mMappedAddress = nullptr;
			mMappedSpan = std::span<std::uint8_t>{};
		}
	}
}