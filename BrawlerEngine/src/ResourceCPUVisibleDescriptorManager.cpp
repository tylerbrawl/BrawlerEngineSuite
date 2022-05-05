module;
#include <cstdint>
#include <array>
#include <optional>
#include <cassert>
#include "DxDef.h"

module Brawler.ResourceCPUVisibleDescriptorManager;
import Brawler.I_GPUResource;
import Util.Engine;

namespace Brawler
{
	ResourceCPUVisibleDescriptorManager::ResourceCPUVisibleDescriptorManager(I_GPUResource& resource) :
#ifdef _DEBUG
		mDescriptorInitializedArr(),
#endif // _DEBUG
		mOwningResource(&resource),
		mCPUVisibleHeap(nullptr)
	{}

	void ResourceCPUVisibleDescriptorManager::Initialize()
	{
		CreateCPUVisibleDescriptorHeap();
		CreateCPUVisibleDescriptors();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE ResourceCPUVisibleDescriptorManager::GetCPUVisibleDescriptorHandle(const ResourceDescriptorType descriptorType) const
	{
#ifdef _DEBUG
		assert(mDescriptorInitializedArr[std::to_underlying(descriptorType)] && "ERROR: An attempt was made to get a CPU-visible descriptor handle, but the CPU-visible descriptor was never created!");
#endif // _DEBUG

		CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor{ mCPUVisibleHeap->GetCPUDescriptorHandleForHeapStart() };
		hCPUDescriptor.Offset(std::to_underlying(descriptorType), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		return hCPUDescriptor;
	}

	void ResourceCPUVisibleDescriptorManager::CreateCPUVisibleDescriptors()
	{
		// Create any applicable descriptors within the CPU-visible descriptor heap.

#ifdef _DEBUG
		mDescriptorInitializedArr = std::array<bool, 3>{ false, false, false };
#endif // _DEBUG

		{
			// Creating a CBV...

			const Brawler::D3D12_RESOURCE_DESC resourceDesc{ mOwningResource->GetResourceDescription() };

			if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
					.BufferLocation = mOwningResource->GetD3D12Resource().GetGPUVirtualAddress(),
					.SizeInBytes = static_cast<std::uint32_t>(resourceDesc.Width)
				};

				Util::Engine::GetD3D12Device().CreateConstantBufferView(&cbvDesc, GetCPUVisibleDescriptorHandle(ResourceDescriptorType::CBV));

#ifdef _DEBUG
				mDescriptorInitializedArr[static_cast<std::size_t>(ResourceDescriptorType::CBV)] = true;
#endif // _DEBUG
			}
		}

		{
			// Creating an SRV...

			const std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc{ mOwningResource->GetSRVDescription() };

			if (srvDesc.has_value())
			{
				Util::Engine::GetD3D12Device().CreateShaderResourceView(&(mOwningResource->GetD3D12Resource()), &(*srvDesc), GetCPUVisibleDescriptorHandle(ResourceDescriptorType::SRV));

#ifdef _DEBUG
				mDescriptorInitializedArr[static_cast<std::size_t>(ResourceDescriptorType::SRV)] = true;
#endif // _DEBUG
			}
				
		}

		{
			// Creating a UAV...

			const std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc{ mOwningResource->GetUAVDescription() };

			if (uavDesc.has_value())
			{
				// Check if there is a counter present for this UAV. The UAV must be for a buffer
				// and uavDesc->Buffer.CounterOffsetInBytes must be a multiple of
				// D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT greater than 0.
				if (uavDesc->ViewDimension == D3D12_UAV_DIMENSION_BUFFER && uavDesc->Buffer.CounterOffsetInBytes > 0)
				{
					// According to the MSDN, raw buffer views are not compatible with UAV counters.
					assert(((uavDesc->Buffer.Flags & ~D3D12_BUFFER_UAV_FLAG_RAW) == uavDesc->Buffer.Flags) && "ERROR: UAV counters cannot be used with raw buffer views!");

					Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
						&(mOwningResource->GetD3D12Resource()),
						&(mOwningResource->GetD3D12Resource()),
						&(*uavDesc),
						GetCPUVisibleDescriptorHandle(ResourceDescriptorType::UAV)
					);
				}
				else
					Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
						&(mOwningResource->GetD3D12Resource()),
						nullptr,
						&(*uavDesc),
						GetCPUVisibleDescriptorHandle(ResourceDescriptorType::UAV)
					);

#ifdef _DEBUG
				mDescriptorInitializedArr[static_cast<std::size_t>(ResourceDescriptorType::UAV)] = true;
#endif // _DEBUG
			}
		}
	}

	void ResourceCPUVisibleDescriptorManager::CreateCPUVisibleDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC cpuHeapDesc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 3,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0
		};

		CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&cpuHeapDesc, IID_PPV_ARGS(&mCPUVisibleHeap)));
	}
}