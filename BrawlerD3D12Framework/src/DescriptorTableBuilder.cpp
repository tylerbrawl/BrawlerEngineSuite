module;
#include <cassert>
#include <optional>
#include <variant>
#include <mutex>
#include "DxDef.h"

module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Util.Engine;
import Util.General;
import Util.D3D12;
import Brawler.D3D12.GPUVendor;
import Brawler.Timer;

namespace Brawler
{
	namespace D3D12
	{
		DescriptorTableBuilder::DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors) :
			mStagingHeap(nullptr),
			mDescriptorTable(),
			mDescriptorInfoArr(),
			mNumDescriptors(tableSizeInDescriptors),
			mTableCreationCritSection()
		{
			// Create the non-shader-visible descriptor heap for staging the descriptors.
			const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = tableSizeInDescriptors,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};

			Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mStagingHeap)));

			mDescriptorInfoArr.resize(tableSizeInDescriptors);
		}

		DescriptorTableBuilder::DescriptorTableBuilder(DescriptorTableBuilder&& rhs) noexcept :
			mStagingHeap(nullptr),
			mDescriptorTable(),
			mDescriptorInfoArr(),
			mNumDescriptors(rhs.mNumDescriptors),
			mTableCreationCritSection()
		{
			const std::scoped_lock<std::mutex> lock{ rhs.mTableCreationCritSection };

			mStagingHeap = std::move(rhs.mStagingHeap);

			mDescriptorTable = std::move(rhs.mDescriptorTable);
			rhs.mDescriptorTable.reset();

			mDescriptorInfoArr = std::move(rhs.mDescriptorInfoArr);

			rhs.mNumDescriptors = 0;
		}

		DescriptorTableBuilder& DescriptorTableBuilder::operator=(DescriptorTableBuilder&& rhs) noexcept
		{
			const std::scoped_lock<std::mutex, std::mutex> lock{ mTableCreationCritSection, rhs.mTableCreationCritSection };

			mStagingHeap = std::move(rhs.mStagingHeap);

			mDescriptorTable = std::move(rhs.mDescriptorTable);
			rhs.mDescriptorTable.reset();

			mDescriptorInfoArr = std::move(rhs.mDescriptorInfoArr);

			mNumDescriptors = rhs.mNumDescriptors;
			rhs.mNumDescriptors = 0;

			return *this;
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

		CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorTableBuilder::GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mStagingHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}

		void DescriptorTableBuilder::CreateDescriptorTable()
		{
			// *LOCKED*
			//
			// This function is called from a locked context.
			
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
			// *LOCKED*
			//
			// This function is called from a locked context.
			
			const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
				.BufferLocation = cbvInfo.BufferSubAllocationPtr->GetGPUVirtualAddress() + cbvInfo.OffsetFromSubAllocationStart,
				.SizeInBytes = static_cast<std::uint32_t>(cbvInfo.BufferSubAllocationPtr->GetSubAllocationSize())
			};
			
			Util::Engine::GetD3D12Device().CreateConstantBufferView(&cbvDesc, GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, const SRVInfo& srvInfo)
		{
			// *LOCKED*
			//
			// This function is called from a locked context.
			
			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(srvInfo.GPUResourcePtr->GetD3D12Resource()), &(srvInfo.SRVDesc), GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, const UAVInfo& uavInfo)
		{
			// *LOCKED*
			//
			// This function is called from a locked context.
			
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