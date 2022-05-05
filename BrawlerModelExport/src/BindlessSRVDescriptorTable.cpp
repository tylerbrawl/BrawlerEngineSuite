module;

module Brawler.D3D12.GPUResourceDescriptors.BindlessSRVDescriptorTable;
import Brawler.D3D12.GPUResourceDescriptors.GPUResourceDescriptorHeap;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		BindlessSRVDescriptorTable::BindlessSRVDescriptorTable() :
			I_DescriptorTable(DescriptorHandleInfo{
				.HCPUDescriptor{Util::Engine::GetGPUResourceDescriptorHeap().GetCPUDescriptorHandle()},
				.HGPUDescriptor{Util::Engine::GetGPUResourceDescriptorHeap().GetGPUDescriptorHandle()}
			})
		{}

		bool BindlessSRVDescriptorTable::IsDescriptorTableValid() const
		{
			// The bindless SRV descriptor table is a dedicated segment of the GPUResourceDescriptorHeap,
			// and is thus always valid.
			return true;
		}
	}
}