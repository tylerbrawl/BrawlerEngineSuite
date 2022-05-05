module;
#include <concepts>

export module Brawler.AssetTypeMap;
import Brawler.AssetTypeID;
import Brawler.I_Asset;
import Brawler.I_PersistentAsset;

export namespace Brawler
{
	class StreamedAudio;
}

namespace IMPL
{
	template <typename T>
	struct AssetTypeMap
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: A mapping was not provided in AssetTypeMap.ixx between an AssetTypeID and its associated asset type!");
	};

	template <>
	struct AssetTypeMap<Brawler::I_Asset>
	{
		static constexpr Brawler::AssetTypeID TYPE_ID = Brawler::AssetTypeID::COUNT_OR_ERROR;
	};

	template <>
	struct AssetTypeMap<Brawler::I_PersistentAsset>
	{
		static constexpr Brawler::AssetTypeID TYPE_ID = Brawler::AssetTypeID::COUNT_OR_ERROR;
	};

	template <>
	struct AssetTypeMap<Brawler::StreamedAudio>
	{
		static constexpr Brawler::AssetTypeID TYPE_ID = Brawler::AssetTypeID::STREAMED_AUDIO;
	};
}

export namespace Brawler
{
	namespace AssetTypeMap
	{
		template <typename T>
			requires std::derived_from<T, I_Asset>
		constexpr Brawler::AssetTypeID GetAssetTypeID()
		{
			return IMPL::AssetTypeMap<T>::TYPE_ID;
		}
	}
}