module;
#include "DxDef.h"

export module Brawler.D3D12.BorrowedD3D12ResourceContainer;
import Brawler.D3D12.D3D12ResourceContainer;

/*
Unlike the other D3D12ResourceContainer derived types, BorrowedD3D12ResourceContainer does
*NOT* own the ID3D12Resource objects associated with it. Instead, it simply references them
and assumes that some other component of the system owns the resource.

This container should only be used in very rare cases, the most prominent one being getting
references to the ID3D12Resource objects behind the back buffers in an IDXGISwapChain instance.
*/

export namespace Brawler
{
	namespace D3D12
	{
		class BorrowedD3D12ResourceContainer final : public D3D12ResourceContainer
		{
		public:
			explicit BorrowedD3D12ResourceContainer(I_GPUResource& owningResource);

			void BorrowD3D12Resource(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResourcePtr);
		};
	}
}