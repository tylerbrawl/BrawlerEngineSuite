module;

export module Brawler.D3D12.FreeGPUResidencyInfo;
import Brawler.ThreadSafeVector;
import Brawler.D3D12.GPUMemoryBudgetInfo;

export namespace Brawler
{
	namespace D3D12
	{
		class I_PageableGPUObject;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		enum class FreeGPUResidencyResult
		{
			SUCCESS,
			ERROR_CANNOT_FREE_OBJECTS
		};

		struct FreeGPUResidencyInfo
		{
			const Brawler::ThreadSafeVector<I_PageableGPUObject*>& PageableObjectArr;
			GPUMemoryBudgetInfo BudgetInfo;
		};
	}
}