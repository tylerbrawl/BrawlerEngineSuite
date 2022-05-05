module;
#include <cstddef>

export module Util.Math;

export namespace Util
{
	namespace Math
	{
		constexpr std::size_t KilobytesToBytes(const std::size_t numKilobytes);
		constexpr std::size_t MegabytesToBytes(const std::size_t numMegabytes);
	}
}

// -------------------------------------------------------------------------------

namespace Util
{
	namespace Math
	{
		constexpr std::size_t KilobytesToBytes(const std::size_t numKilobytes)
		{
			return (numKilobytes * 1024);
		}

		constexpr std::size_t MegabytesToBytes(const std::size_t numMegabytes)
		{
			return (numMegabytes * 1024 * 1024);
		}
	}
}