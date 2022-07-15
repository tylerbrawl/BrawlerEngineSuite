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
		constexpr Int3 TEST_3{};
		constexpr Float4 TEST_F4{};

		constexpr Float3x3 TEST_3X3{};
		
		constexpr Float4x4 IDENTITY_MATRIX_4X4{ DirectX::XMFLOAT4X4{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		} };

		constexpr float PI = 3.14159265359f;
		constexpr float HALF_PI = (PI / 2.0f);

		constexpr Quaternion IDENTITY_QUATERNION{ DirectX::XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
	}
}