module;
#include <string>
#include <cstdint>

export module Brawler.FilePathHash;

namespace
{
	consteval std::uint64_t CreateFilePathHash(const std::wstring_view filePath)
	{
		// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...

		std::uint64_t hash = 5381;

		for (const auto& c : filePath)
			hash = (((hash << 5) + hash) ^ c);

		return hash;
	}
}

export namespace Brawler
{
	class FilePathHash
	{
	public:
		// By forcing the constructor which takes a std::wstring_view to be consteval, we
		// ensure that the hash is computed at compile time. With proper usage, the file
		// path string is completely absent at runtime, and isn't even present in the
		// compiled executable!
		//
		// What defines "proper usage," you ask? Just follow these rules:
		//
		//   1. Whenever you would ordinarily use a file path string, take a const FilePathHash&
		//      instead.
		//
		//   2. Create all FilePathHash instances using the std::wstring_view constructor as
		//      constexpr. (The compiler won't even allow you to do otherwise, anyways.)
		consteval FilePathHash(const std::wstring_view filePath);

		// In cases where a file path hash is read directly from a binary file, this
		// constructor can instead be used.
		explicit FilePathHash(const std::uint64_t pathHash);

		std::uint64_t GetHash() const;
		operator std::uint64_t() const;

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
}