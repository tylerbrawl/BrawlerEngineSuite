module;
#include <filesystem>
#include "DxDef.h"

export module Brawler.FileMapper;
import Brawler.FileAccessMode;
import Brawler.MappedFileView;

export namespace Brawler
{
	class FileMapper
	{
	public:
		FileMapper(const std::filesystem::path& filePath, const FileAccessMode accessMode);
		~FileMapper();

		FileMapper(const FileMapper& rhs) = delete;
		FileMapper& operator=(const FileMapper& rhs) = delete;

		FileMapper(FileMapper&& rhs) noexcept;
		FileMapper& operator=(FileMapper&& rhs) noexcept;

		MappedFileView CreateMappedFileView(const std::uint64_t fileOffsetInBytes, const std::uint64_t viewSizeInBytes) const;

		/// <summary>
		/// Retrieves the offset into a file closest to fileOffsetInBytes which is aligned to
		/// (i.e., a multiple of) the Win32 page alignment.
		/// </summary>
		/// <param name="fileOffsetInBytes">
		/// - The offset, in bytes, from the start of a file which is to be aligned.
		/// </param>
		/// <returns>
		/// This function returns the closest aligned file offset which comes before
		/// fileOffsetInBytes.
		/// </returns>
		static std::uint64_t GetAlignedFileOffset(const std::uint64_t fileOffsetInBytes);

		FileAccessMode GetFileAccessMode() const;
		HANDLE GetMappingObjectHandle() const;

	private:
		void CloseHandles();

	private:
		FileAccessMode mAccessMode;
		HANDLE mHFile;
		HANDLE mHMappingObject;
	};
}