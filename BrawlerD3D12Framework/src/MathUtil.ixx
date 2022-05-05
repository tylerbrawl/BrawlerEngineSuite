module;
#include <cstdint>
#include <type_traits>
#include <intrin.h>
#include <cassert>

export module Util.Math;

namespace Util
{
	namespace Math
	{
		template <typename... Args>
		concept OnlyIntegralArguments = (std::is_integral_v<Args> && ...);
	}
}

export namespace Util
{
	namespace Math
	{
		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr std::uint64_t KilobytesToBytes(const T kilobytes);
		
		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr std::uint64_t MegabytesToBytes(const T megabytes);

		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr bool IsPowerOfTwo(const T value);

		/// <summary>
		/// Given a value, this function returns either the provided value if it is already a
		/// power of two or the next highest power-of-two value after it.
		/// 
		/// *WARNING*: If the next highest power of two cannot be expressed with an unsigned
		/// 64-bit integer, then this function will return 0.
		/// </summary>
		/// <typeparam name="T">
		/// - The provided type must be an unsigned integer value.
		/// </typeparam>
		/// <param name="value">
		/// - The value which is to be raised to the next power of two, if it is not already one.
		/// </param>
		/// <returns>
		/// If value is already a power of two, then the function returns value. Otherwise, the
		/// function returns the next highest integer which is a power of two.
		/// 
		/// *WARNING*: If the next highest power of two cannot be expressed with an unsigned
		/// 64-bit integer, then this function will return 0.
		/// </returns>
		template <typename T>
			requires std::is_integral_v<T>
		constexpr std::uint64_t RaiseToPowerOfTwo(const T value);

		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr std::uint64_t CountOneBits(const T value);

		template <typename T, typename U>
			requires OnlyIntegralArguments<T, U>
		__forceinline constexpr std::uint64_t Align(const T valueToAlign, const U alignment);

		template <typename T, typename U>
			requires OnlyIntegralArguments<T, U>
		__forceinline constexpr std::uint64_t AlignToPowerOfTwo(const T valueToAlign, const U alignment);
	}
}

// --------------------------------------------------------------------------------------------------------

namespace Util
{
	namespace Math
	{
		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr std::uint64_t KilobytesToBytes(const T kilobytes)
		{
			return (static_cast<std::uint64_t>(kilobytes) * 1024);
		}
		
		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr std::uint64_t MegabytesToBytes(const T megabytes)
		{
			return (static_cast<std::uint64_t>(megabytes) * 1024 * 1024);
		}

		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr bool IsPowerOfTwo(const T value)
		{
			return (CountOneBits(value) == 1);
		}

		template <typename T>
			requires std::is_integral_v<T>
		constexpr std::uint64_t RaiseToPowerOfTwo(const T value)
		{
			const std::uint64_t checkedValue = static_cast<std::uint64_t>(value);

			// If the value is already a power of two, then exit early.
			if (IsPowerOfTwo(checkedValue))
				return checkedValue;

			if (std::is_constant_evaluated())
			{
				std::uint64_t currPowerOfTwo = 1;

				while (currPowerOfTwo < checkedValue && currPowerOfTwo != 0)
					currPowerOfTwo <<= 1;

				return currPowerOfTwo;
			}
			else
			{
				// Find the index of the most significant 1 bit. Then, take the
				// value 1 and shift it left (index + 1) times. For instance,
				// here is the value 12 in binary:
				//
				// 12 = 1100b
				//
				// Starting from the right-most bit with a zero-based index, we
				// find that the most-significant bit is located at index 3.
				// The next highest power of two, 16, is 1 << (3 + 1) = 1 << 4:
				//
				// 1b -> 10b -> 100b -> 1000b -> 10000b
				//
				// 16 = 10000b

				unsigned long bitIndex = 0;
				std::uint8_t result = _BitScanReverse64(&bitIndex, checkedValue);

				if (result != 0)
					return (static_cast<std::uint64_t>(1) << (bitIndex + 1));

				return 1;
			}
		}

		template <typename T>
			requires std::is_integral_v<T>
		__forceinline constexpr std::uint64_t CountOneBits(const T value)
		{
			// We branch here depending on the context of evaluation so that we can use
			// compiler intrinsics at runtime which are not available at compile time.

			if (std::is_constant_evaluated())
			{
				std::uint64_t copiedValue = static_cast<std::uint64_t>(value);
				std::uint64_t bitCount = 0;

				while (copiedValue != 0)
				{
					if ((copiedValue & 0x1) != 0)
						++bitCount;

					copiedValue >>= 1;
				}

				return bitCount;
			}
			else
			{
				return __popcnt64(static_cast<std::uint64_t>(value));
			}
		}

		template <typename T, typename U>
			requires OnlyIntegralArguments<T, U>
		__forceinline constexpr std::uint64_t Align(const T valueToAlign, const U alignment)
		{
			const std::uint64_t largeValueToAlign = static_cast<std::uint64_t>(valueToAlign);
			const std::uint64_t largeAlignment = static_cast<std::uint64_t>(alignment);
			
			const std::uint64_t alignmentRemainder = (largeValueToAlign % largeAlignment);
			return ((alignmentRemainder == 0) ? largeValueToAlign : (largeValueToAlign + (largeAlignment - alignmentRemainder)));
		}

		template <typename T, typename U>
			requires OnlyIntegralArguments<T, U>
		__forceinline constexpr std::uint64_t AlignToPowerOfTwo(const T valueToAlign, const U alignment)
		{
			assert(IsPowerOfTwo(alignment) && "ERROR: An attempt was made to call Util::Math::AlignToPowerOfTwo() with a non-power-of-two alignment!");

			const std::uint64_t largeValueToAlign = static_cast<std::uint64_t>(valueToAlign);
			const std::uint64_t largeAlignment = static_cast<std::uint64_t>(alignment);
			
			return ((largeValueToAlign + (largeAlignment - 1)) & ~(largeAlignment - 1));
		}
	}
}