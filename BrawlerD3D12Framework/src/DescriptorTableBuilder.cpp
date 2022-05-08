module;
#include <cassert>
#include <optional>
#include <variant>
#include "DxDef.h"

module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		DescriptorTableBuilder::DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors) :
			mStagingHeap(nullptr),
			mDescriptorTable(),
			mDescriptorInfoArr(),
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

			mDescriptorInfoArr.resize(tableSizeInDescriptors);
		}

		PerFrameDescriptorTable DescriptorTableBuilder::GetDescriptorTable()
		{
			// Create a new per-frame descriptor table if we do not have one for the
			// current frame.
			if (!mDescriptorTable.has_value() || !mDescriptorTable->IsDescriptorTableValid())
				CreateDescriptorTable();
			
			return *mDescriptorTable;
		}

		std::uint32_t DescriptorTableBuilder::GetDescriptorTableSize() const
		{
			return mNumDescriptors;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorTableBuilder::GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mStagingHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}

		void DescriptorTableBuilder::CreateDescriptorTable()
		{
			// First, create the descriptors within the non-shader-visible heap.
			std::uint32_t currIndex = 0;
			for (const auto& descriptorInfo : mDescriptorInfoArr)
			{
				switch (descriptorInfo.index())
				{
				case 1:
				{
					CreateConstantBufferView(currIndex, std::get<CBVInfo>(descriptorInfo));
					break;
				}

				case 2:
				{
					CreateShaderResourceView(currIndex, std::get<SRVInfo>(descriptorInfo));
					break;
				}

				case 3:
				{
					CreateUnorderedAccessView(currIndex, std::get<UAVInfo>(descriptorInfo));
					break;
				}

				default:
				{
					assert(false && "ERROR: A DescriptorTableBuilder was never assigned a descriptor for all of its entries!");
					std::unreachable();

					break;
				}
				}

				++currIndex;
			}

			// Now, copy the descriptors over to the shader-visible GPUResourceDescriptorHeap.
			// This creates the per-frame descriptor table.
			mDescriptorTable = Util::Engine::GetGPUResourceDescriptorHeap().CreatePerFrameDescriptorTable(*this);

			// We don't want to destroy the data which we used to create the descriptors, because
			// we can still use them to create PerFrameDescriptorTable instances on the next
			// frame.
		}

		void DescriptorTableBuilder::CreateConstantBufferView(const std::uint32_t index, const CBVInfo& cbvInfo)
		{
			const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
				.BufferLocation = cbvInfo.BufferSubAllocationPtr->GetGPUVirtualAddress() + cbvInfo.OffsetFromSubAllocationStart,
				.SizeInBytes = static_cast<std::uint32_t>(cbvInfo.BufferSubAllocationPtr->GetSubAllocationSize())
			};
			
			Util::Engine::GetD3D12Device().CreateConstantBufferView(&cbvDesc, GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, const SRVInfo& srvInfo)
		{
			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(srvInfo.GPUResourcePtr->GetD3D12Resource()), &(srvInfo.SRVDesc), GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, const UAVInfo& uavInfo)
		{
			Brawler::D3D12Resource* const d3dCounterResourcePtr = (uavInfo.UAVCounter.HasValue() ? &(uavInfo.UAVCounter->GetD3D12Resource()) : nullptr);

			Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
				&(uavInfo.GPUResourcePtr->GetD3D12Resource()),
				d3dCounterResourcePtr,
				&(uavInfo.UAVDesc),
				GetCPUDescriptorHandle(index)
			);
		}
	}
}