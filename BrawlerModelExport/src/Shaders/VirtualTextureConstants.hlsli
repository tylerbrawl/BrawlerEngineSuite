namespace VirtualTextures
{
	static const uint2 USEFUL_PAGE_DIMENSIONS = uint2(128, 128);
	
	static const uint POINT_FILTER_TILING_MODE = 0;
	static const uint BILINEAR_FILTER_TILING_MODE = 1;
	static const uint TRILINEAR_FILTER_TILING_MODE = 2;
	static const uint ANISOTROPIC_8X_FILTER_TILING_MODE = 3;

	template <uint TilingModeNum>
	struct TilingTypeInfo
	{};

	template <>
	struct TilingTypeInfo<POINT_FILTER_TILING_MODE>
	{
		static const uint2 BORDER_DIMENSIONS;
		static const uint2 PAGE_DIMENSIONS;
	};

	static const uint2 TilingTypeInfo <POINT_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(0, 0);
	static const uint2 TilingTypeInfo <POINT_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<POINT_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

	template <>
	struct TilingTypeInfo<BILINEAR_FILTER_TILING_MODE>
	{
		static const uint2 BORDER_DIMENSIONS;
		static const uint2 PAGE_DIMENSIONS;
	};

	static const uint2 TilingTypeInfo <BILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(1, 1);
	static const uint2 TilingTypeInfo <BILINEAR_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<BILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

	template <>
	struct TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE>
	{
		static const uint2 BORDER_DIMENSIONS;
		static const uint2 PAGE_DIMENSIONS;
	};

	static const uint2 TilingTypeInfo <TRILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(2, 2);
	static const uint2 TilingTypeInfo <TRILINEAR_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

	template <>
	struct TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE>
	{
		static const uint2 BORDER_DIMENSIONS;
		static const uint2 PAGE_DIMENSIONS;
	};

	static const uint2 TilingTypeInfo <ANISOTROPIC_8X_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(4, 4);
	static const uint2 TilingTypeInfo <ANISOTROPIC_8X_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);
}