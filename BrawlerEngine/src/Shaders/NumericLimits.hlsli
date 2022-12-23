#pragma once

namespace BrawlerHLSL
{
	template <typename T>
	struct NumericLimits
	{};
	
	template <>
	struct NumericLimits<int>
	{
		static const int MIN = -2147483648;
		static const int MAX = 2147483647;
	};
	
	template <>
	struct NumericLimits<uint>
	{
		static const uint MIN = 0;
		static const uint MAX = 0xFFFFFFFF;
	};
}