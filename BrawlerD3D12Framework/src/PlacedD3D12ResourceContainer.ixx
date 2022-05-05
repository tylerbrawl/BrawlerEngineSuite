module;

export module Brawler.D3D12.PlacedD3D12ResourceContainer;
import Brawler.D3D12.D3D12ResourceContainer;
import Brawler.D3D12.GPUResourceAllocationHandle;

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceInitializationInfo;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class PlacedD3D12ResourceContainer final : public D3D12ResourceContainer
		{
		public:
			explicit PlacedD3D12ResourceContainer(I_GPUResource& owningResource);

			void CreatePlacedD3D12Resource(GPUResourceAllocationHandle&& hAllocation, const GPUResourceInitializationInfo& initializationInfo);

		private:
			GPUResourceAllocationHandle mHAllocation;
		};
	}
}