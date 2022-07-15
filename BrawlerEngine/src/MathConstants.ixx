module;
#include <cstdint>
#include <bit>
#include <compare>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.MathConstants;
import Brawler.Math.MathTypes;
import Util.Math;

export namespace Brawler
{
	namespace Math
	{
		// Simply exporting the following was causing the MSVC to crash with a stack overflow error
		// on attempting to use the exported values within other modules:
		//
		// constexpr Float3x3 IDENTITY_MATRIX_3X3{ ... };
		//
		// To work around this issue, the identity matrices are wrapped inside of consteval functions.
		// For some reason, that works just fine.

		consteval Float3x3 GetIdentityMatrix3x3()
		{
			constexpr Float3x3 IDENTITY_MATRIX_3X3{
				1.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 1.0f
			};

			return IDENTITY_MATRIX_3X3;
		}

		consteval Float4x4 GetIdentityMatrix4x4()
		{
			constexpr Float4x4 IDENTITY_MATRIX_4X4{
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};

			return IDENTITY_MATRIX_4X4;
		}

		constexpr float PI = 3.14159265359f;
		constexpr float HALF_PI = (PI / 2.0f);

		constexpr Quaternion IDENTITY_QUATERNION{ DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };

		constexpr Float3 X_AXIS{ DirectX::XMFLOAT3{1.0f, 0.0f, 0.0f} };
		constexpr Float3 Y_AXIS{ DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f} };
		constexpr Float3 Z_AXIS{ DirectX::XMFLOAT3{0.0f, 0.0f, 1.0f} };
	}
}