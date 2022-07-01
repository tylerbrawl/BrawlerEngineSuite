namespace IMPL
{
	namespace Util
	{
		namespace VirtualTexture
		{
			uint GetMergedPageGeometricSum(in const int exponent)
			{
				// The sum of the geometric series is -((1 - 2 ^ (2 * exponent)) / 3).
				
				const int numerator = 1 - (1 << (exponent << 1));
				return uint(-(numerator / 3));
			}
		}
	}
}

namespace Util
{
	namespace VirtualTexture
	{
		uint2 CalculateLogicalMergedPageStartCoordinates(in const int mipLevel, in const uint mipLevel0LogicalSize)
		{
			uint2 startCoordinates = { mipLevel0LogicalSize, mipLevel0LogicalSize };
			
			startCoordinates.x >>= max((mipLevel - 1) - (mipLevel & 0x1), 0);
			startCoordinates.y >>= (max(mipLevel - 1, 0) & 0xFFFFFFFE);
			
			startCoordinates.x *= IMPL::Util::VirtualTexture::GetMergedPageGeometricSum(mipLevel >> 1);
			startCoordinates.y *= IMPL::Util::VirtualTexture::GetMergedPageGeometricSum((mipLevel + 1) >> 1);
			
			return startCoordinates;
		}
	}
}