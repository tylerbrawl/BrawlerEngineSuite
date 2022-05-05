module;
#include <span>
#include "DxDef.h"

export module Brawler.MappedFileView;
import Brawler.FileAccessMode;

export namespace Brawler
{
	class FileMapper;
}

export namespace Brawler
{
	class MappedFileView
	{
	public:
		struct ViewParams
		{
			const FileMapper& Mapper;
			std::uint64_t FileOffsetInBytes;
			std::uint64_t ViewSizeInBytes;
		};

	public:
		MappedFileView();
		explicit MappedFileView(const ViewParams& params);
		~MappedFileView();

		MappedFileView(const MappedFileView& rhs) = delete;
		MappedFileView& operator=(const MappedFileView& rhs) = delete;

		MappedFileView(MappedFileView&& rhs) noexcept;
		MappedFileView& operator=(MappedFileView&& rhs) noexcept;

		std::span<std::uint8_t> GetMappedData();
		std::span<const std::uint8_t> GetMappedData() const;

	private:
		void UnmapView();

	private:
		std::uint8_t* mMappedAddress;
		std::span<std::uint8_t> mMappedSpan;
	};
}