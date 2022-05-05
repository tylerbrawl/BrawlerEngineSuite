module;
#include <cstdint>
#include <type_traits>
#include <cassert>

export module Util.Math;

export namespace Util
{
	namespace Math
	{
		constexpr std::uint64_t KilobytesToBytes(const std::uint32_t kilobytes);
		constexpr std::uint64_t MegabytesToBytes(const std::uint32_t megabytes);
		constexpr std::uint64_t GigabytesToBytes(const std::uint32_t gigabytes);

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr std::uint32_t CountOneBits(T value);

		template <typename T>
			requires std::is_integral_v<T>
		constexpr T AlignUp(const T valueToAlign, const T alignment);

		template <typename T>
			requires std::is_integral_v<T>
		constexpr bool IsAligned(const T valueToAlign, const T alignment);

		template <typename T>
			requires std::is_integral_v<T>
		constexpr T GetNextPowerOfTwo(const T value);
	}
}

// ---------------------------------------------------------------------------------------

namespace Util
{
	namespace Math
	{
		constexpr std::uint64_t KilobytesToBytes(const std::uint32_t kilobytes)
		{
			return (static_cast<std::uint64_t>(kilobytes) * 1024);
		}

		constexpr std::uint64_t MegabytesToBytes(const std::uint32_t megabytes)
		{
			return (static_cast<std::uint64_t>(megabytes) * 1024 * 1024);
		}

		constexpr std::uint64_t GigabytesToBytes(const std::uint32_t gigabytes)
		{
			return (static_cast<std::uint64_t>(gigabytes) * 1024 * 1024 * 1024);
		}

		template <typename T>
			requires std::is_arithmetic_v<T>
		constexpr std::uint32_t CountOneBits(T value)
		{
			std::uint32_t oneBitsCount = 0;

			while (value)
			{
				if (value & 0x1)
					++oneBitsCount;

				value >>= 1;
			}

			return oneBitsCount;
		}

		template <typename T>
			requires std::is_integral_v<T>
		constexpr T AlignUp(const T valueToAlign, const T alignment)
		{
			// This only works if alignment is a power of 2.
			// 
			// TODO: Does this assert even fire for constexpr evaluations? This seems like the perfect
			// use for C++23 if consteval...
			assert(CountOneBits(alignment) == 1 && "ERROR: An attempt was made to call Util::Math::AlignUp() with a non-power-of-two alignment!");

			return ((valueToAlign + (alignment - 1)) & ~(alignment - 1));
		}

		template <typename T>
			requires std::is_integral_v<T>
		constexpr bool IsAligned(const T valueToAlign, const T alignment)
		{
			return (AlignUp(valueToAlign, alignment) == valueToAlign);
		}

		template <typename T>
			requires std::is_integral_v<T>
		constexpr T GetNextPowerOfTwo(const T value)
		{
			if (!value)
				return 0;

			T powerOfTwo = 1;

			while (powerOfTwo < value)
				powerOfTwo <<= 1;

			return powerOfTwo;
		}
	}
}