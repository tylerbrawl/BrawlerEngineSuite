module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Win32.CreateWindowInfo;

export namespace Brawler
{
	namespace Win32
	{
		struct CreateWindowInfo
		{
			std::uint32_t WindowStyle;
			std::uint32_t WindowStyleEx;
			
			DirectX::XMINT2 WindowStartCoordinates;
			DirectX::XMUINT2 WindowSize;
		};
	}
}