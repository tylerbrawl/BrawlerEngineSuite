module;
#include "DxDef.h"

export module Brawler.D3DVideoBudgetInfo;

export namespace Brawler
{
	struct D3DVideoBudgetInfo
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO GPUBudgetInfo;
		DXGI_QUERY_VIDEO_MEMORY_INFO CPUBudgetInfo;
	};
}