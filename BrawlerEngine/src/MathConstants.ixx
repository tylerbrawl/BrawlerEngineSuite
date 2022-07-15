module;
#include <cstdint>
#include <bit>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.MathConstants;
import Brawler.Math.MathTypes;
import Util.Math;

export namespace Brawler
{
	namespace Math
	{
		// Simply exporting the following was causing the MSVC to crash with an internal compiler
		// error:
		//
		// constexpr Float3x3 IDENTITY_MATRIX_3X3{ ... };
		//
		// In fact, attempting to export any constexpr matrix type other than Float4x4 result in
		// the crash. As a workaround, we define consteval functions which return the corresponding
		// matrices.

		constexpr Float3x3 IDENTITY_MATRIX_3X3{
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f
		};

		constexpr Float4x4 IDENTITY_MATRIX_4X4{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		constexpr float PI = 3.14159265359f;
		constexpr float HALF_PI = (PI / 2.0f);

		constexpr Quaternion IDENTITY_QUATERNION{ DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
	}
}