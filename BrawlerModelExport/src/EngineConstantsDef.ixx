module;
#include <compare>

export module Brawler.EngineConstants:EngineConstantsDef;
import Brawler.NZStringView;

export namespace Brawler
{
	namespace D3D12
	{
		consteval Brawler::NZWStringView GetPSOCacheFileName()
		{
			return Brawler::NZWStringView{ L"Model Exporter PSO Cache.bpl" };
		}
	}
}