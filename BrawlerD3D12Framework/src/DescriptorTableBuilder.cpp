module;
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Util.Engine;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		DescriptorTableBuilder::DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors) :
			mStagingHeap(nullptr),
			mNumDescriptors(tableSizeInDescriptors)
		{
			// Create the non-shader-visible descriptor heap for staging the descriptors.
			const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = tableSizeInDescriptors,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};

			CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mStagingHeap)));
		}

		PerFrameDescriptorTable DescriptorTableBuilder::FinalizeDescriptorTable() const
		{
			return Util::Engine::GetGPUResourceDescriptorHeap().CreatePerFrameDescriptorTable(*this);
		}

		std::uint32_t DescriptorTableBuilder::GetDescriptorTableSize() const
		{
			return mNumDescriptors;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorTableBuilder::GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mStagingHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}

		void DescriptorTableBuilder::CreateConstantBufferView(const std::uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
		{
			assert(cbvDesc.BufferLocation != 0 && "ERROR: An attempt was made to create a CBV within a descriptor heap, but a nullptr buffer GPU virtual address was provided!");

			Util::Engine::GetD3D12Device().CreateConstantBufferView(&cbvDesc, GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, const SRVInfo& srvInfo)
		{
			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(srvInfo.D3DResource), &(srvInfo.SRVDesc), GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, const UAVInfo& uavInfo)
		{
			Brawler::D3D12Resource* const d3dCounterResourcePtr = (uavInfo.UAVCounterD3DResource.HasValue() ? &(*(uavInfo.UAVCounterD3DResource)) : nullptr);

			Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
				&(uavInfo.D3DResource),
				d3dCounterResourcePtr,
				&(uavInfo.UAVDesc),
				GetCPUDescriptorHandle(index)
			);
		}
	}
}