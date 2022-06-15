module;
#include <string>

export module Brawler.CondensedByteArrayInfo;

export namespace Brawler
{
	struct CondensedByteArrayInfo
	{
		/// <summary>
		/// This string represents the contents of an initializer list which can be used to
		/// instantiate a std::array<std::uint64_t, InitializerListLengthInElements> with
		/// the contents of the relevant data.
		/// </summary>
		std::string UInt64ByteArrayInitializerListContents;

		/// <summary>
		/// This is the number of individual elements contained within the initializer list
		/// whose contents are those of UInt64ByteArrayInitializerListContents. This value
		/// should be used as the size of the std::array<std::uint64_t> which represents
		/// binary data.
		/// </summary>
		std::size_t InitializerListLengthInElements;

		/// <summary>
		/// It may not happen that the size of the data is evenly distributed by 8. In that
		/// case, the last entry in UInt64ByteArrayInitializerListContents may include extra
		/// zeroes at the end which do not belong to the actual data. This value represents
		/// the actual size, in bytes, of the data.
		/// </summary>
		std::size_t ActualDataSizeInBytes;
	};
}