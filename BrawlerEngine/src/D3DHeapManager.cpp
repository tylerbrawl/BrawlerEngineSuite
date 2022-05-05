module;
#include <type_traits>
#include <memory>
#include <unordered_map>

module Brawler.D3DHeapManager;
import Util.General;
import Brawler.D3DHeapPool;
import Brawler.ResourceCreationInfo;

namespace Brawler
{
	D3DHeapManager::D3DHeapManager() :
		mHeapPoolMap()
	{
		for (std::underlying_type_t<Brawler::AllowedD3DResourceType> i = 0; i < Util::General::EnumCast(Brawler::AllowedD3DResourceType::COUNT_OR_ERROR); ++i)
		{
			const Brawler::AllowedD3DResourceType allowedType = static_cast<Brawler::AllowedD3DResourceType>(i);
			mHeapPoolMap.emplace(allowedType, allowedType);
		}
	}
}