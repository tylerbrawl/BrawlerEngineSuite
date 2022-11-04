namespace Util
{
	namespace Math
	{
		template <typename T>
		T HeavisideStepFunction(in const T value)
		{
			// The HLSL step() intrinsic performs ((x >= y) ? 1 : 0), but we want
			// ((value > 0) ? 1 : 0). We might be able to do step(EPSILON, value),
			// but I don't really see a point.
			
			return (value > T(0) ? T(1) : T(0));
		}
	}
}