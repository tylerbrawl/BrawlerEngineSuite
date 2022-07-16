module;
#include <cstdint>

module Brawler.ViewComponent;
import Brawler.Math.MathConstants;
import Util.Math;

namespace
{
	// ==================================================================================================================
	// View Matrix Defaults
	// ==================================================================================================================

	// There's no "right" answer for the starting view direction. In my opinion, however, having the
	// view look down the +Z-axis makes the most sense.
	constexpr Brawler::Math::Float3 DEFAULT_VIEW_DIRECTION{ Brawler::Math::Z_AXIS };
	static_assert(DEFAULT_VIEW_DIRECTION.IsNormalized(), "ERROR: The default view direction vector should be normalized!");

	constexpr Brawler::Math::Float3 DEFAULT_WORLD_UP_DIRECTION{ Brawler::Math::Y_AXIS };
	static_assert(DEFAULT_WORLD_UP_DIRECTION.IsNormalized(), "ERROR: The default world up direction vector should be normalized!");
	
	consteval Brawler::Math::Quaternion GetDefaultViewSpaceQuaternion()
	{
		const Brawler::Math::Float3 cameraPositiveZAxis{ DEFAULT_VIEW_DIRECTION };
		const Brawler::Math::Float3 cameraPositiveXAxis{ DEFAULT_WORLD_UP_DIRECTION.Cross(cameraPositiveZAxis) };
		const Brawler::Math::Float3 cameraPositiveYAxis{ cameraPositiveZAxis.Cross(cameraPositiveXAxis) };

		const Brawler::Math::Float3x3 cameraSpaceOrthonormalBasis{
			cameraPositiveXAxis.GetX(), cameraPositiveXAxis.GetY(), cameraPositiveXAxis.GetZ(),
			cameraPositiveYAxis.GetX(), cameraPositiveYAxis.GetY(), cameraPositiveYAxis.GetZ(),
			cameraPositiveZAxis.GetX(), cameraPositiveZAxis.GetY(), cameraPositiveZAxis.GetZ()
		};

		return Brawler::Math::Quaternion{ cameraSpaceOrthonormalBasis };
	}

	static constexpr Brawler::Math::Quaternion DEFAULT_VIEW_SPACE_QUATERNION{ GetDefaultViewSpaceQuaternion() };

	// ==================================================================================================================
	// Projection Matrix Defaults
	// ==================================================================================================================
	static constexpr bool DEFAULT_USE_REVERSE_Z_VALUE = true;

	static constexpr float DEFAULT_NEAR_PLANE_DISTANCE_METERS = 0.005f;  // n = 0.5 cm
	static constexpr float DEFAULT_FAR_PLANE_DISTANCE_METERS = 5000.0f;  // f = 5 km

	static constexpr float ARC_COS_TEST = Util::Math::GetArcCosineValue(-0.428f);

	template <float Value>
	struct Assert
	{
		static_assert(sizeof(Value) != sizeof(Value));
	};

	static constexpr Assert<ARC_COS_TEST> ASSERTION{};
}

namespace Brawler
{
	

}