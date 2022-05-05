module;
#include "DxDef.h"

export module Brawler.D3D12.GPUMemoryBudgetInfo;

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUMemoryBudgetInfo
		{
			/// <summary>
			/// This field describes the budget for device-local video memory, that is, memory
			/// local to the video adapter. This segment represents the fastest available memory
			/// to the GPU.
			/// 
			/// It is advisable to stick to this budget for maximum performance, and only spill
			/// into non-local memory in dire situations.
			/// </summary>
			DXGI_QUERY_VIDEO_MEMORY_INFO DeviceLocalMemoryInfo;

			/// <summary>
			/// This field describes the budget for non-local video memory, that is, memory not
			/// local to the video adapter. Memory in this segment may (will) have slower performance
			/// compared to memory in the device-local segment.
			/// </summary>
			DXGI_QUERY_VIDEO_MEMORY_INFO SystemMemoryInfo;
		};
	}
}