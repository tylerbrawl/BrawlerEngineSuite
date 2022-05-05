module;
#include <array>
#include <string>
#include <span>

export module Brawler.SHA512Hash;
import Util.Engine;

export namespace Brawler
{
	class SHA512Hash
	{
	public:
		SHA512Hash();
		explicit SHA512Hash(std::array<std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES>&& byteArr);

		SHA512Hash(const SHA512Hash& rhs) = default;
		SHA512Hash& operator=(const SHA512Hash& rhs) = default;

		SHA512Hash(SHA512Hash&& rhs) noexcept = default;
		SHA512Hash& operator=(SHA512Hash&& rhs) noexcept = default;

		bool operator==(const SHA512Hash& rhs) const;
		std::strong_ordering operator<=>(const SHA512Hash& rhs) const;

		/// <summary>
		/// Creates a std::string which represents the SHA-512 hash in text format. (Leading zeroes
		/// are removed.)
		/// </summary>
		/// <returns>
		/// The function returns the equivalent SHA-512 hash in text format.
		/// </returns>
		std::string ToString() const;

		std::span<const std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES> GetByteArray() const;

	private:
		std::array<std::uint8_t, Util::Engine::SHA_512_HASH_SIZE_IN_BYTES> mByteArr;
	};
}