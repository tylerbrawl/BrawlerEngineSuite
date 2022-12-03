module;
#include "DxDef.h"

module Brawler.D3D12.BorrowedD3D12ResourceContainer;

namespace Brawler
{
	namespace D3D12
	{
		BorrowedD3D12ResourceContainer::BorrowedD3D12ResourceContainer(I_GPUResource& owningResource) :
			D3D12ResourceContainer(owningResource)
		{}

		void BorrowedD3D12ResourceContainer::BorrowD3D12Resource(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResourcePtr)
		{
			SetD3D12Resource(std::move(d3dResourcePtr));
		}
	}
}