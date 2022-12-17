module;
#include <string>
#include <cstdint>

export module Brawler.FilePathHash;

namespace
{
	constexpr std::uint64_t CreateFilePathHash(const std::wstring_view filePath)
	{
		// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...

		std::uint64_t hash = 5381;

		for (const auto c : filePath)
			hash = (((hash << 5) + hash) ^ c);

		return hash;
	}

	static constexpr std::string_view HEX_CHARACTER_STRING = "0123456789ABCDEF";

	constexpr std::string GetByteHexString(const std::uint8_t byte)
	{
		// Performance Tip: For the MSVC STL, *NEVER* use std::stringstream in
		// multi-threaded applications, even if it is just for std::hex(). If you
		// do, then benchmark your application to check for critical section contention.

		std::string hexStr{};

		hexStr += HEX_CHARACTER_STRING[byte >> 4];
		hexStr += HEX_CHARACTER_STRING[byte & 0xF];

		return hexStr;
	}
}

namespace Brawler
{
	struct AllowRuntimeFilePathHashConstruction_T
	{};
}

export namespace Brawler
{
	inline constexpr AllowRuntimeFilePathHashConstruction_T CONSTRUCT_FILE_PATH_HASH_AT_RUNTIME{};
}

export namespace Brawler
{
	class FilePathHash
	{
	public:
		FilePathHash() = default;
		
		// By forcing the constructor which takes a std::wstring_view to be consteval, we
		// ensure that the hash is computed at compile time. With proper usage, the file
		// path string is completely absent at runtime, and isn't even present in the
		// compiled executable!
		//
		// What defines "proper usage," you ask? Just follow these rules:
		//
		//   1. Whenever you would ordinarily use a file path string, take a const FilePathHash
		//      instead.
		//
		//   2. Create all FilePathHash instances using the std::wstring_view constructor as
		//      constexpr. (The compiler won't even allow you to do otherwise, anyways.)
		consteval FilePathHash(const std::wstring_view filePath);

		// When it is necessary, however, we will also allow construction of FilePathHash instances
		// from strings at runtime. This must be done explicitly using the tag object
		// CONSTRUCT_FILE_PATH_HASH_AT_RUNTIME.
		constexpr FilePathHash(const AllowRuntimeFilePathHashConstruction_T, const std::wstring_view filePath);

		// In cases where a file path hash is read directly from a binary file, this
		// constructor can instead be used.
		constexpr explicit FilePathHash(const std::uint64_t pathHash);

		constexpr std::uint64_t GetHash() const;
		constexpr operator std::uint64_t() const;

		constexpr std::string GetHashStringLittleEndian() const;
		constexpr std::string GetHashStringBigEndian() const;

	private:
		std::uint64_t mHash;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	consteval FilePathHash::FilePathHash(const std::wstring_view filePath) :
		mHash(CreateFilePathHash(filePath))
	{}

	constexpr FilePathHash::FilePathHash(const AllowRuntimeFilePathHashConstruction_T, const std::wstring_view filePath) :
		mHash(CreateFilePathHash(filePath))
	{}

	constexpr FilePathHash::FilePathHash(const std::uint64_t pathHash) :
		mHash(pathHash)
	{}

	constexpr std::uint64_t FilePathHash::GetHash() const
	{
		return mHash;
	}

	constexpr FilePathHash::operator std::uint64_t() const
	{
		return mHash;
	}

	constexpr std::string FilePathHash::GetHashStringLittleEndian() const
	{
		// We know this is just 8, but still...
		constexpr std::size_t BYTE_COUNT = sizeof(mHash) / sizeof(std::uint8_t);
		std::string hashStr{};

		for (std::size_t i = 0; i < BYTE_COUNT; ++i)
		{
			const std::uint8_t currByte = static_cast<std::uint8_t>((mHash >> (i * 8)) & 0xFF);
			hashStr += GetByteHexString(currByte);
		}

		return hashStr;
	}

	constexpr std::string FilePathHash::GetHashStringBigEndian() const
	{
		// We know this is just 8, but still...
		constexpr std::size_t BYTE_COUNT = sizeof(mHash) / sizeof(std::uint8_t);
		std::string hashStr{};

		for (std::size_t i = 0; i < BYTE_COUNT; ++i)
		{
			const std::uint8_t currByte = static_cast<std::uint8_t>((mHash >> (i * 8)) & 0xFF);
			hashStr = (GetByteHexString(currByte) + hashStr);
		}

		return hashStr;
	}
}