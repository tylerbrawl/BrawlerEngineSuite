module;
#include <string>

export module Brawler.StringHasher;

export namespace Brawler
{
	/// <summary>
	/// This is a helper constexpr class for constructing a hash from a string. The actual hash generated
	/// is *NOT* cryptographically secure!
	/// </summary>
	class StringHasher
	{
	public:
		template <typename CharT, typename Traits>
		constexpr StringHasher(const std::basic_string_view<CharT, Traits> str);

		std::uint64_t GetHash() const;

	private:
		const std::uint64_t mHash;
	};
}

// ---------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename CharT, typename Traits>
	constexpr StringHasher::StringHasher(const std::basic_string_view<CharT, Traits> str) :
		mHash([] (const std::basic_string_view<CharT, Traits> str)
		{
			// Shamelessly copied from http://www.cse.yorku.ca/~oz/hash.html...

			std::uint64_t hash = 5381;

			for (const auto& c : str)
				hash = (((hash << 5) + hash) ^ c);

			return hash;
		}(str))
	{}

	std::uint64_t StringHasher::GetHash() const
	{
		return mHash;
	}
}