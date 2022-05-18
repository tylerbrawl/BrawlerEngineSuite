module;
#include <string>
#include <cstdint>
#include <functional>

export module Brawler.FilePathHash;

namespace
{
	constexpr std::uint64_t CreateFilePathHash(const std::wstring_view filePath)
	{
		// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...

		std::uint64_t hash = 5381;

		for (const auto& c : filePath)
			hash = (((hash << 5) + hash) ^ c);

		return hash;
	}

	static constexpr std::string_view HEX_CHARACTER_STRING = "0123456789ABCDEF";

	std::string GetByteHexString(const std::uint8_t byte)
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

export namespace Brawler
{
	class FilePathHash
	{
	public:
		FilePathHash() = default;

		// For our tools, we do not need to have this constructor be consteval. In
		// fact, doing so would make it impossible to create hash values for strings
		// taken from arbitrary data.
		explicit FilePathHash(const std::wstring_view filePath);

		// In cases where a file path hash is read directly from a binary file, this
		// constructor can instead be used.
		explicit FilePathHash(const std::uint64_t pathHash);

		std::uint64_t GetHash() const;
		operator std::uint64_t() const;
		std::string GetHashString() const;

	private:
		std::uint64_t mHash;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	FilePathHash::FilePathHash(const std::wstring_view filePath) :
		mHash(CreateFilePathHash(filePath))
	{}

	FilePathHash::FilePathHash(const std::uint64_t pathHash) :
		mHash(pathHash)
	{}

	std::uint64_t FilePathHash::GetHash() const
	{
		return mHash;
	}

	FilePathHash::operator std::uint64_t() const
	{
		return mHash;
	}

	std::string FilePathHash::GetHashString() const
	{
		// We know this is just 8, but still...
		static constexpr std::size_t BYTE_COUNT = sizeof(mHash) / sizeof(std::uint8_t);
		std::string hashStr{};

		for (std::size_t i = 0; i < BYTE_COUNT; ++i)
		{
			const std::uint8_t currByte = static_cast<std::uint8_t>((mHash >> (i * 8)) & 0xFF);
			hashStr += GetByteHexString(currByte);
		}

		return hashStr;
	}
}

export namespace std
{
	template <>
	struct hash<Brawler::FilePathHash>
	{
		hash() = default;

		std::size_t operator()(const Brawler::FilePathHash filePathHash) const
		{
			return filePathHash.GetHash();
		}
	};
}