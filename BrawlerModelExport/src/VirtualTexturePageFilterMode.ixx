module;

export module Brawler.VirtualTexturePageFilterMode;

export namespace Brawler
{
	enum class VirtualTexturePageFilterMode
	{
		POINT_FILTER,
		BILINEAR_FILTER,
		TRILINEAR_FILTER,
		ANISOTROPIC_8X_FILTER
	};
}