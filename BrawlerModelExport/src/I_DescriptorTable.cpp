module;
#include <cassert>
#include <utility>

module Brawler.D3D12.GPUResourceDescriptors.I_DescriptorTable;

namespace Brawler
{
	namespace D3D12
	{
		I_DescriptorTable::I_DescriptorTable(DescriptorHandleInfo&& handleInfo) :
			mHandleInfo(std::move(handleInfo))
		{}

		const DescriptorHandleInfo& I_DescriptorTable::GetDescriptorHandleInfo() const
		{
			assert(IsDescriptorTableValid() && "ERROR: An attempt was made to access the handles of a descriptor table after it was invalidated!");
			return mHandleInfo;
		}
	}
}