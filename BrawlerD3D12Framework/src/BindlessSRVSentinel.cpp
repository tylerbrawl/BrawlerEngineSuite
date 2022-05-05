module;
#include <optional>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.BindlessSRVSentinel;
import Util.Engine;
import Brawler.D3D12.GPUResourceDescriptorHeap;

namespace Brawler
{
	namespace D3D12
	{
		BindlessSRVSentinel::BindlessSRVSentinel(const std::uint32_t bindlessSRVIndex) :
			mBindlessSRVIndex(bindlessSRVIndex),
			mSRVDesc()
		{}
		
		BindlessSRVSentinel::~BindlessSRVSentinel()
		{
			ReturnBindlessSRVIndex();
		}

		BindlessSRVSentinel::BindlessSRVSentinel(BindlessSRVSentinel&& rhs) noexcept :
			mBindlessSRVIndex(std::move(rhs.mBindlessSRVIndex)),
			mSRVDesc(std::move(rhs.mSRVDesc))
		{
			rhs.mBindlessSRVIndex.reset();
		}

		BindlessSRVSentinel& BindlessSRVSentinel::operator=(BindlessSRVSentinel&& rhs) noexcept
		{
			ReturnBindlessSRVIndex();

			mBindlessSRVIndex = std::move(rhs.mBindlessSRVIndex);
			rhs.mBindlessSRVIndex.reset();

			mSRVDesc = std::move(rhs.mSRVDesc);

			return *this;
		}

		void BindlessSRVSentinel::SetSRVDescription(D3D12_SHADER_RESOURCE_VIEW_DESC&& srvDesc)
		{
			mSRVDesc = std::move(srvDesc);
		}

		std::uint32_t BindlessSRVSentinel::GetBindlessSRVIndex() const
		{
			assert(mBindlessSRVIndex.has_value());
			return *mBindlessSRVIndex;
		}

		void BindlessSRVSentinel::UpdateBindlessSRV(Brawler::D3D12Resource& d3dResource) const
		{
			assert(mBindlessSRVIndex.has_value() && "ERROR: An attempt was made to write a bindless SRV into the GPUResourceDescriptorHeap, but the corresponding BindlessSRVSentinel was never allocated a bindless SRV index!");

			const CD3DX12_CPU_DESCRIPTOR_HANDLE hBindlessSRVHandle{ Util::Engine::GetGPUResourceDescriptorHeap().GetCPUDescriptorHandle(*mBindlessSRVIndex) };
			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(d3dResource), &mSRVDesc, hBindlessSRVHandle);
		}

		void BindlessSRVSentinel::ReturnBindlessSRVIndex()
		{
			if (mBindlessSRVIndex.has_value())
			{
				Util::Engine::GetGPUResourceDescriptorHeap().ReClaimBindlessSRV(*this);
				mBindlessSRVIndex.reset();
			}
		}
	}
}