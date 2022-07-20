module;
#include <array>
#include <DxDef.h>

export module Brawler.GlobalTextureCollection;

namespace Brawler
{
	template <DXGI_FORMAT Format>
	struct GlobalTextureCollectionInfo
	{
		static_assert(sizeof(Format) != sizeof(Format), "ERROR: An explicit template specialization of GlobalTextureCollectionInfo was never provided for a given DXGI_FORMAT! (See GlobalTextureCollection.ixx.)");
	};

	template <>
	struct GlobalTextureCollectionInfo<DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB>
	{
		static constexpr std::size_t GLOBAL_TEXTURE_COUNT = 1;
	};
}

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