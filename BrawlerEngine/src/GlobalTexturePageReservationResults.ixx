module;
#include <vector>
#include <memory>
#include <DxDef.h>

export module Brawler.GlobalTexturePageReservationResults;
import Brawler.GlobalTexturePageSwapOperation;

export namespace Brawler
{
	struct GlobalTexturePageReservationResults
	{
		std::vector<std::unique_ptr<GlobalTexturePageSwapOperation>> ActivePageSwapArr;
		HRESULT HResult;
	};
}