module;
#include <array>
#include <DxDef.h>

export module Brawler.GlobalTextureCollection;
import Brawler.GlobalTextureFormatInfo;

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	class GlobalTextureCollection
	{
	public:
		GlobalTextureCollection() = default;

		GlobalTextureCollection(const GlobalTextureCollection& rhs) = delete;
		GlobalTextureCollection& operator=(const GlobalTextureCollection& rhs) = delete;

		GlobalTextureCollection(GlobalTextureCollection&& rhs) noexcept = default;
		GlobalTextureCollection& operator=(GlobalTextureCollection&& rhs) noexcept = default;


	};
}