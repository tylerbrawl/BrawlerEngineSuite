module;
#include <compare>
#include "Quaternion.h"

export module Brawler.Math.MathTypes;

/*
Initially, Matrix, Vector, and Quaternion were all module partition units of Brawler.Math.MathTypes. After a
while, however, I got so sick of dealing with all of the internal compiler errors that I just moved them into
a header file.

Now, Brawler.Math.MathTypes exists only to export valid instantiations of the math types defined in the header
files. Some day, the MSVC will be able to support my templated madness without crashing all of the time...
*/

export namespace Brawler
{
	namespace Math
	{
		using Float3x3 = Matrix<3, 3>;
		using Float3x4 = Matrix<3, 4>;
		using Float4x3 = Matrix<4, 3>;
		using Float4x4 = Matrix<4, 4>;
	}
}

export namespace Brawler
{
	namespace Math
	{
		using Int2 = Vector<std::int32_t, 2>;
		using UInt2 = Vector<std::uint32_t, 2>;
		using Float2 = Vector<float, 2>;

		using Int3 = Vector<std::int32_t, 3>;
		using UInt3 = Vector<std::uint32_t, 3>;
		using Float3 = Vector<float, 3>;

		using Int4 = Vector<std::int32_t, 4>;
		using UInt4 = Vector<std::uint32_t, 4>;
		using Float4 = Vector<float, 4>;
	}
}

export namespace Brawler
{
	namespace Math
	{
		using Quaternion = IMPL::Quaternion;
	}
}