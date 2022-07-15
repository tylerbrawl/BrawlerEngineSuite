module;
#include <cstdint>
#include <type_traits>
#include <intrin.h>
#include <cassert>
#include <optional>
#include <cmath>

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
		__forceinline constexpr std::uint64_t GigabytesToBytes(const T gigabytes);

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

		template <typename T>
			requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t))
		__forceinline constexpr std::uint32_t GetFirstSetBit(const T valueToScan);

		/// <summary>
		/// Calculates the square root of the provided value. Unlike std::sqrtf() (as of writing this), this
		/// function is constexpr. At runtime, however, it simply calls std::sqrtf(), anyways.
		/// 
		/// NOTE: C++ paper P0533R9 advocates for constexpr for both <cmath> and <cstdlib>, and has been
		/// accepted by the standards committee. Once the MSVC STL implements this feature, please use
		/// std::sqrtf() directly, instead.
		/// </summary>
		/// <typeparam name="T">
		/// - The type of the value for which the square root is to be calculated.
		/// </typeparam>
		/// <param name="value">
		/// - The value for which the square root is to be calculated.
		/// </param>
		/// <returns></returns>
		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float GetSquareRoot(const T value);

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float GetSineAngle(const T angleInRadians);

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float RadiansToDegrees(const T angleInRadians);

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float DegreesToRadians(const T angleInDegrees);
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
		__forceinline constexpr std::uint64_t GigabytesToBytes(const T gigabytes)
		{
			return (static_cast<std::uint64_t>(gigabytes) * 1024 * 1024 * 1024);
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

		template <typename T>
			requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t))
		__forceinline constexpr std::uint32_t GetFirstSetBit(const T valueToScan)
		{
			assert(valueToScan != 0 && "ERROR: A value of 0 was provided to Util::Math::GetFirstSetBit()!");
			
			if (std::is_constant_evaluated())
			{
				std::uint32_t currIndex = 0;

				while (valueToScan != 0)
				{
					if ((valueToScan & 0x1) != 0)
						return currIndex;

					++currIndex;
				}

				// We shouldn't ever get here.
				assert(false);
			}
			else
			{
				auto bitScanResult = 0;
				unsigned long index = 0;

				if constexpr (sizeof(valueToScan) <= sizeof(std::uint32_t))
					bitScanResult = _BitScanForward(&index, valueToScan);

				else
					bitScanResult = _BitScanForward64(&index, valueToScan);

				assert(bitScanResult != 0);
				return index;
			}
		}

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float GetSquareRoot(const T value)
		{
			if (std::is_constant_evaluated())
			{
				if (value == 0) [[unlikely]]
					return 0.0f;

				if (value < 0) [[unlikely]]
					return NAN;

				// std::sqrtf() isn't constexpr yet at the time of writing this. Ready to have some fun?

				// Start with an initial estimate which will be used for later refinement. Let m = a * (2^(2n))
				// be the value which we want to calculate the square root for. By exploiting the layout
				// of a floating-point value as defined in IEEE-754, we can get these values fairly easily.
				const float castedValue = static_cast<float>(value);
				const float halfExponent = (static_cast<float>(((std::bit_cast<std::int32_t>(castedValue) >> 23) & 0x8) - 127) / 2.0f);

				// value == a * (2^(2n)) == a * (2^n) * (2^n). We have n == halfExponent already, so let's 
				// represent 2^n as a float.
				const std::int32_t biasedHalfExponent = static_cast<std::int32_t>(halfExponent) + 127;
				const float twoRaisedToHalfExponent = std::bit_cast<float>(biasedHalfExponent << 23);

				const float a = castedValue / (twoRaisedToHalfExponent * twoRaisedToHalfExponent);
				const float initialEstimate = ((0.485f + (0.485f * a)) * twoRaisedToHalfExponent);

				// Use the Babylonian Method to converge to the correct result.
				constexpr float MAXIMUM_ALLOWED_DIFFERENCE = 0.001f;
				float prevEstimate = initialEstimate;
				float currEstimate = 0.0f;
				float currDifference = 0.0f;

				do
				{
					currEstimate = 0.5f * (prevEstimate + (castedValue / prevEstimate));
					currDifference = currEstimate - prevEstimate;

					if (currDifference < 0.0f)
						currDifference *= -1.0f;

					prevEstimate = currEstimate;
				} while (currDifference > MAXIMUM_ALLOWED_DIFFERENCE);

				return currEstimate;
			}
			else
				return std::sqrtf(static_cast<float>(value));
		}

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float GetSineAngle(const T angleInRadians)
		{
			if (std::is_constant_evaluated())
			{
				// In a constant-evaluated context, we will use the power series definition of sin(x)
				// to converge to the correct result.

				constexpr auto FACTORIAL_LAMBDA = [] (const std::size_t startingValue)
				{
					std::size_t currFactorialValue = 1;

					for (std::size_t currValue = startingValue; currValue > 1; --currValue)
						currFactorialValue *= currValue;

					return currFactorialValue;
				};

				constexpr float MAXIMUM_ALLOWED_DIFFERENCE = 0.001f;

				float currSineValue = 0.0f;
				float prevSineValue = currSineValue;
				std::uint32_t currIteration = 0;

				const std::int32_t angleFloatExponent = ((std::bit_cast<std::int32_t>(angleInRadians) >> 23) & 0xFF) - 127;
				const float angleNoExponent = std::bit_cast<float>(std::bit_cast<std::int32_t>(angleInRadians) & 0x807FFFFF);

				while (true)
				{
					const std::uint32_t iterationExponentValue = ((2 * currIteration) + 1);

					// Get the value of halfAngle^(currExponentValue). By exploiting the layout of IEEE-754
					// floating-point values, we can calculate this in constant time.
					const std::uint32_t newAngleFloatExponent = (angleFloatExponent * iterationExponentValue) + 127;

					float currScaledHalfAngle = std::bit_cast<float>(std::bit_cast<std::int32_t>(angleNoExponent) | (newAngleFloatExponent << 23));
					currScaledHalfAngle *= (currIteration % 2 == 0 ? 1.0f : -1.0f);
					currScaledHalfAngle /= FACTORIAL_LAMBDA(iterationExponentValue);

					currSineValue += currScaledHalfAngle;

					float sineValueDifference = (currSineValue - prevSineValue);

					if (sineValueDifference < 0.0f)
						sineValueDifference *= -1.0f;

					if (sineValueDifference < MAXIMUM_ALLOWED_DIFFERENCE)
						return currSineValue;

					prevSineValue = currSineValue;
					++currIteration;
				}
			}
			else
				return std::sinf(static_cast<float>(angleInRadians));
		}

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float RadiansToDegrees(const T angleInRadians)
		{
			constexpr float PI = 3.14159265359f;
			constexpr float RADIANS_TO_DEGREES_CONVERSION_FACTOR = (180.0f / PI);

			return (static_cast<float>(angleInRadians) * RADIANS_TO_DEGREES_CONVERSION_FACTOR);
		}

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr float DegreesToRadians(const T angleInDegrees)
		{
			constexpr float PI = 3.14159265359f;
			constexpr float DEGREES_TO_RADIANS_CONVERSION_FACTOR = (PI / 180.0f);

			return (static_cast<float>(angleInDegrees) * DEGREES_TO_RADIANS_CONVERSION_FACTOR);
		}
	}
}