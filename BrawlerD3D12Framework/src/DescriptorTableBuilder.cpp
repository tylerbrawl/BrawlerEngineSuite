module;
#include <cassert>
#include <optional>
#include <variant>
#include <mutex>
#include "DxDef.h"

module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Util.General;
import Util.D3D12;
import Brawler.D3D12.DescriptorHandleInfo;

namespace Brawler
{
	namespace D3D12
	{
		DescriptorTableBuilder::DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors) :
			mDescriptorTable(),
			mDescriptorInfoArr(),
			mNumDescriptors(tableSizeInDescriptors),
			mTableCreationCritSection()
		{
			mDescriptorInfoArr.resize(tableSizeInDescriptors);
		}

		DescriptorTableBuilder::DescriptorTableBuilder(DescriptorTableBuilder&& rhs) noexcept :
			mDescriptorTable(),
			mDescriptorInfoArr(),
			mNumDescriptors(rhs.mNumDescriptors),
			mTableCreationCritSection()
		{
			const std::scoped_lock<std::mutex> lock{ rhs.mTableCreationCritSection };

			mDescriptorTable = std::move(rhs.mDescriptorTable);
			rhs.mDescriptorTable.reset();

			mDescriptorInfoArr = std::move(rhs.mDescriptorInfoArr);

			rhs.mNumDescriptors = 0;
		}

		DescriptorTableBuilder& DescriptorTableBuilder::operator=(DescriptorTableBuilder&& rhs) noexcept
		{
			const std::scoped_lock<std::mutex, std::mutex> lock{ mTableCreationCritSection, rhs.mTableCreationCritSection };

			mDescriptorTable = std::move(rhs.mDescriptorTable);
			rhs.mDescriptorTable.reset();

			mDescriptorInfoArr = std::move(rhs.mDescriptorInfoArr);

			mNumDescriptors = rhs.mNumDescriptors;
			rhs.mNumDescriptors = 0;

			return *this;
		}

		void DescriptorTableBuilder::NullifyConstantBufferView(const std::uint32_t index)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::NullifyConstantBufferView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());
			
			// Don't bother setting a NULL descriptor if we do not need to.
			const bool hasTier3Support = (Util::Engine::GetGPUCapabilities().GPUResourceBindingTier == ResourceBindingTier::TIER_3);

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			if (hasTier3Support) [[likely]]
				mDescriptorInfoArr[index] = ResourceBindingTier3NullDescriptor{};
			else [[unlikely]]
			{
				// The specs state that if we define SizeInBytes to be 0, then we can specify nullptr (0) for
				// BufferLocation and still get the intended behavior of a null CBV. (The source for this
				// information is https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#null-descriptors.
				// Don't bother looking on the MSDN for it; it's not there.)
				mDescriptorInfoArr[index] = D3D12_CONSTANT_BUFFER_VIEW_DESC{
					.BufferLocation = 0,
					.SizeInBytes = 0
				};
			}
		}

		PerFrameDescriptorTable DescriptorTableBuilder::GetDescriptorTable()
		{
			// Create a new per-frame descriptor table if we do not have one for the
			// current frame.
			//
			// Even with the std::mutex, there is a condition we need to consider.
			// Suppose that thread A is creating a PerFrameDescriptorTable for frame N,
			// while thread B is creating a PerFrameDescriptorTable for frame (N + 1).
			// This might happen, for instance, if thread A gets suspended long enough
			// for thread B to call DescriptorTableBuilder::GetDescriptorTable() while
			// recording a RenderPass before thread A ever has a chance to.
			//
			// Should we be worried about this? The answer is ultimately no, I believe.
			// If thread B gets here first, then CreateDescriptorTable() will create
			// a descriptor table in the per-frame descriptor segment of the
			// GPUResourceDescriptorHeap for frame (N + 1). Thread A will then find that
			// mDescriptorTable->IsDescriptorTableValid() returns true because frame (N + 1)
			// is later than what Util::Engine::GetCurrentFrameNumber() would return for
			// that thread (i.e., frame N).
			//
			// So, thread A would end up using the descriptor table for frame (N + 1).
			// This isn't a bad thing, however, because the GPU will still be able to
			// use the descriptor table as usual. Since descriptors are created immediately
			// on the CPU timeline, we know that even though the descriptors are meant for
			// frame (N + 1), they will still be usable on the GPU timeline for frame N.
			//
			// Thus, we conclude that this race condition is benign. (In practice, it is
			// unlikely to happen, anyways. Still, it is important to consider these types of
			// things to ensure correctness.)
			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			if (!mDescriptorTable.has_value() || !mDescriptorTable->IsDescriptorTableValid())
				CreateDescriptorTable();
			
			return *mDescriptorTable;
		}

		std::uint32_t DescriptorTableBuilder::GetDescriptorTableSize() const
		{
			return mNumDescriptors;
		}

		void DescriptorTableBuilder::CreateDescriptorTable()
		{
			// *LOCKED*
			//
			// This function is called from a locked context.

			// Originally, I wanted to create a non-shader-visible heap, write all of the descriptors into that heap,
			// and then perform a copy into the GPUResourceDescriptorHeap instance. As it turns out, however, this isn't
			// necessarily practical because there appears to be a hard limit on the number of non-shader-visible
			// descriptor heaps which can be created. This isn't documented anywhere on the MSDN, so I am assuming that
			// this is a driver limitation, but who really knows?

			// Reserve the required amount of descriptors directly from the GPUResourceDescriptorHeap.
			const DescriptorHandleInfo perFrameReservationHandleInfo{ Util::Engine::GetGPUResourceDescriptorHeap().CreatePerFrameDescriptorHeapReservation(GetDescriptorTableSize()) };
			const std::uint32_t handleIncrementSize = Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			
			// For each element in mDescriptorInfoArr, create its corresponding descriptor directly within the
			// GPUResourceDescriptorHeap.
			std::uint32_t currIndex = 0;
			for (const auto& descriptorInfo : mDescriptorInfoArr)
			{
				const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor{ perFrameReservationHandleInfo.HCPUDescriptor, static_cast<std::int32_t>(currIndex), handleIncrementSize };
				
				switch (descriptorInfo.index())
				{
				case 1:
				{
					CreateConstantBufferView(hCPUDescriptor, std::get<D3D12_CONSTANT_BUFFER_VIEW_DESC>(descriptorInfo));
					break;
				}

				case 2:
				{
					CreateShaderResourceView(hCPUDescriptor, std::get<SRVInfo>(descriptorInfo));
					break;
				}

				case 3:
				{
					CreateUnorderedAccessView(hCPUDescriptor, std::get<UAVInfo>(descriptorInfo));
					break;
				}

				case 4:
				{
					assert(Util::Engine::GetGPUCapabilities().GPUResourceBindingTier == ResourceBindingTier::TIER_3 && "ERROR: A NULL descriptor was marked as skipped in a DescriptorTableBuilder instance, but the current device does not have Resource Binding Tier 3 support!");
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

			// Create the PerFrameDescriptorTable which represents the descriptors for the current
			// frame.
			mDescriptorTable = std::optional<PerFrameDescriptorTable>{ std::in_place, PerFrameDescriptorTable::InitializationInfo{
				.HandleInfo{ perFrameReservationHandleInfo },
				.CurrentFrameNumber = Util::Engine::GetCurrentFrameNumber()
			} };

			// We don't want to destroy the data which we used to create the descriptors, because
			// we can still use them to create PerFrameDescriptorTable instances on the next
			// frame.
		}

		void DescriptorTableBuilder::CreateConstantBufferView(const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
		{
			// *LOCKED*
			//
			// This function is called from a locked context.

			// We actually do not need to do anything special for null descriptors, since cbvDesc will already
			// be appropriately set in that case.

			Util::Engine::GetD3D12Device().CreateConstantBufferView(&cbvDesc, hCPUDescriptor);
		}

		void DescriptorTableBuilder::CreateShaderResourceView(const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor, const SRVInfo& srvInfo)
		{
			// *LOCKED*
			//
			// This function is called from a locked context.
			
			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(srvInfo.GPUResourcePtr->GetD3D12Resource()), &(srvInfo.SRVDesc), hCPUDescriptor);
		}

		void DescriptorTableBuilder::CreateUnorderedAccessView(const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor, const UAVInfo& uavInfo)
		{
			// *LOCKED*
			//
			// This function is called from a locked context.

			if (uavInfo.GPUResourcePtr != nullptr) [[likely]]
			{
				Brawler::D3D12Resource* const d3dCounterResourcePtr = (uavInfo.UAVCounterResource.HasValue() ? &(*(uavInfo.UAVCounterResource)) : nullptr);

				Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
					&(uavInfo.GPUResourcePtr->GetD3D12Resource()),
					d3dCounterResourcePtr,
					&(uavInfo.UAVDesc),
					hCPUDescriptor
				);
			}
			else [[unlikely]]
			{
				Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
					nullptr,
					nullptr,
					&(uavInfo.UAVDesc),
					hCPUDescriptor
				);
			}
		}
	}
}