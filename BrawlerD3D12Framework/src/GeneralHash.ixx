module;
#include <cstdint>
#include <span>
#include <ranges>
#include <bit>

export module Brawler.GeneralHash;

namespace Brawler
{
	template <typename T>
	std::uint32_t Compute32BitMurmurHash3(const T& key) noexcept
	{
		// The implementation of this algorithm was basically copied from https://en.wikipedia.org/wiki/MurmurHash.

		static constexpr std::uint32_t SEED = 0xDEADBEEF;
		static constexpr std::uint32_t C1 = 0xCC9E2D51;
		static constexpr std::uint32_t C2 = 0x1B873593;
		static constexpr std::uint32_t R1 = 15;
		static constexpr std::uint32_t R2 = 13;
		static constexpr std::uint32_t M = 5;
		static constexpr std::uint32_t N = 0xE6546B64;

		std::uint32_t hash = SEED;
		const std::span<const std::byte> keyByteSpan{ std::as_bytes(std::span<const T>{ &key, 1 }) };

		static constexpr std::size_t NUM_COMPLETE_FOUR_BYTE_CHUNKS = (sizeof(T) / 4);

		for (const auto fourByteChunk : keyByteSpan | std::views::chunk(4) | std::views::take(NUM_COMPLETE_FOUR_BYTE_CHUNKS))
		{
			const std::span<const std::byte> chunkSpan{ fourByteChunk };
			std::uint32_t k = ((static_cast<std::uint32_t>(chunkSpan[0]) << 24) | (static_cast<std::uint32_t>(chunkSpan[1]) << 16) | (static_cast<std::uint32_t>(chunkSpan[2]) << 8) | (static_cast<std::uint32_t>(chunkSpan[3])));

			k *= C1;
			k = std::rotl(k, R1);
			k *= C2;

			hash ^= k;
			hash = std::rotl(hash, R2);
			hash = ((hash * M) + N);
		}

		static constexpr std::size_t NUM_EVALUATED_BYTES = (NUM_COMPLETE_FOUR_BYTE_CHUNKS * 4);
		static constexpr std::size_t REMAINING_BYTES = (sizeof(T) - NUM_EVALUATED_BYTES);
		if constexpr (REMAINING_BYTES > 0)
		{
			for (const auto nByteChunk : keyByteSpan | std::views::drop(NUM_EVALUATED_BYTES) | std::views::chunk(4))
			{
				std::uint32_t remainingBytes = 0;

				{
					std::uint32_t shiftAmount = 24;

					for (const auto byte : nByteChunk)
					{
						remainingBytes |= (static_cast<std::uint32_t>(byte) << shiftAmount);
						shiftAmount -= 8;
					}
				}

				remainingBytes *= C1;
				remainingBytes = std::rotl(remainingBytes, R1);
				remainingBytes *= C2;

				hash ^= remainingBytes;
			}
		}

		hash ^= sizeof(T);

		hash ^= (hash >> 16);
		hash *= 0x85EBCA6B;
		hash ^= (hash >> 13);
		hash *= 0xC2B2AE35;
		hash ^= (hash >> 16);

		return hash;
	}

	template <typename T>
	std::uint32_t Compute32BitFNV1AHash(const T& key) noexcept
	{
		// This implementation was also copied, this time from https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function.
		
		static constexpr std::uint32_t FNV_OFFSET_BASIS = 0x811C9DC5;
		static constexpr std::uint32_t FNV_PRIME = 0x01000193;

		std::uint32_t hash = FNV_OFFSET_BASIS;
		const std::span<const std::byte> keyByteSpan{ std::as_bytes(std::span<const T>{ &key, 1 }) };

		for (const auto byte : keyByteSpan)
		{
			hash ^= byte;
			hash *= FNV_PRIME;
		}

		return hash;
	}
}

export namespace Brawler
{
	template <typename T>
	struct GeneralHash
	{
		std::size_t operator()(const T& key) const noexcept
		{
			// The idea for this comes from a random Stack Overflow comment, which can be found at
			// https://security.stackexchange.com/questions/209882/can-a-32-bit-hash-be-made-into-a-64-bit-hash-by-calling-it-twice-with-different#comment525387_210049.
			// I have no idea how well this will actually work.

			return ((static_cast<std::size_t>(Compute32BitMurmurHash3(key)) << 32) | (static_cast<std::size_t>(Compute32BitFNV1AHash(key))));
		}
	};
}