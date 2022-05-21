module;
#include <variant>
#include <optional>
#include <cassert>
#include <mutex>
#include "DxDef.h"

export module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.PerFrameDescriptorTable;
import Brawler.OptionalRef;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.UAVCounterSubAllocation;

namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class DescriptorTableBuilder
		{
		private:
			friend class GPUResourceDescriptorHeap;

		private:
			struct CBVInfo
			{
				const I_BufferSubAllocation* BufferSubAllocationPtr;
				std::size_t OffsetFromSubAllocationStart;
			};

			struct SRVInfo
			{
				const I_GPUResource* GPUResourcePtr;
				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			};

			struct UAVInfo
			{
				const I_GPUResource* GPUResourcePtr;

				/// <summary>
				/// If this member is left empty, then no UAV counter is added to the created
				/// UAV. Otherwise, the specified Brawler::D3D12Resource is used as the UAV
				/// counter.
				/// 
				/// Note that only buffers can have a UAV counter, and there are requirements
				/// which are validated by the D3D12 runtime when using UAV counters. More information
				/// about this can be found at 
				/// https://docs.microsoft.com/en-us/windows/win32/direct3d12/uav-counters#using-uav-counters.
				/// </summary>
				Brawler::OptionalRef<const UAVCounterSubAllocation> UAVCounter;

				D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			};

			using DescriptorInfoVariant = std::variant<std::monostate, CBVInfo, SRVInfo, UAVInfo>;

		public:
			DescriptorTableBuilder() = delete;
			explicit DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors);

			DescriptorTableBuilder(const DescriptorTableBuilder& rhs) = delete;
			DescriptorTableBuilder& operator=(const DescriptorTableBuilder& rhs) = delete;

			DescriptorTableBuilder(DescriptorTableBuilder&& rhs) noexcept;
			DescriptorTableBuilder& operator=(DescriptorTableBuilder&& rhs) noexcept;

			template <typename DataElementType>
			void CreateConstantBufferView(const std::uint32_t index, const ConstantBufferView<DataElementType> cbv);

			template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
			void CreateShaderResourceView(const std::uint32_t index, const ShaderResourceView<Format, ViewDimension>& srv);

			template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
			void CreateUnorderedAccessView(const std::uint32_t index, const UnorderedAccessView<Format, ViewDimension>& uav);

			/// <summary>
			/// If this is the first time which this function is called for this DescriptorTableBuilder
			/// instance on a given frame, then it will allocate from the GPUResourceDescriptorHeap's per-frame
			/// descriptor segment and create the descriptors within it. After that, the return value
			/// of this function is the table which was created for the remainder of the frame.
			/// 
			/// The DescriptorTableBuilder::Create*View() functions do not immediately create D3D12
			/// descriptors; instead, they copy information which will be used to actually create
			/// the descriptors once DescriptorTableBuilder::GetDescriptorTable() is called. This
			/// allows for DescriptorTableBuilder instances to be created before the I_GPUResource
			/// instances which it is creating descriptors for have been assigned ID3D12Resource
			/// instances. For this reason, it is the caller's responsibility to ensure that
			/// I_GPUResource instances for which descriptors are to be made outlive this
			/// DescriptorTableBuilder instance.
			/// 
			/// Although this function returns a PerFrameDescriptorTable instance, it is valid to
			/// both retain this DescriptorTableBuilder instance and call this function again across
			/// multiple frames. A new PerFrameDescriptorTable instance is created on the first call
			/// of this function on any given frame. However, it is *NOT* valid to persist the
			/// created PerFrameDescriptorTable instances across multiple frames.
			/// 
			/// *NOTE*: To ensure that descriptors can be created, this function should only be
			/// called after all of the relevant I_GPUResource instances have had ID3D12Resource
			/// instances assigned to them. For both persistent and transient resources, this is guaranteed
			/// to be the case during RenderPass recording time on the CPU timeline; that is, it
			/// is valid to call this function within a callback which is passed to
			/// RenderPass::SetRenderPassCommands().
			/// </summary>
			/// <returns>
			/// The function returns a PerFrameDescriptorTable which contains the descriptors
			/// described by this DescriptorTableBuilder.
			/// </returns>
			PerFrameDescriptorTable GetDescriptorTable();

			/// <summary>
			/// Retrieves the size of the descriptor table.
			/// 
			/// NOTE: The returned value does *NOT* equal the number of descriptors which were
			/// created via calls to CreateConstantBufferView(), CreateShaderResourceView(), or
			/// CreateUnorderedAccessView()! It only describes the number of descriptors which
			/// the table is capable of supporting.
			/// </summary>
			/// <returns>
			/// The function returns the size of the descriptor table.
			/// </returns>
			std::uint32_t GetDescriptorTableSize() const;

		private:
			void CreateDescriptorTable();

			void CreateConstantBufferView(const std::uint32_t index, const CBVInfo& cbvInfo);
			void CreateShaderResourceView(const std::uint32_t index, const SRVInfo& srvInfo);
			void CreateUnorderedAccessView(const std::uint32_t index, const UAVInfo& uavInfo);

			CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mStagingHeap;
			std::optional<PerFrameDescriptorTable> mDescriptorTable;
			std::vector<DescriptorInfoVariant> mDescriptorInfoArr;
			std::uint32_t mNumDescriptors;
			mutable std::mutex mTableCreationCritSection;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DataElementType>
		void DescriptorTableBuilder::CreateConstantBufferView(const std::uint32_t index, const ConstantBufferView<DataElementType> cbv)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::CreateConstantBufferView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };
			
			mDescriptorInfoArr[index] = CBVInfo{
				.BufferSubAllocationPtr{ &(cbv.GetBufferSubAllocation()) },
				.OffsetFromSubAllocationStart = cbv.GetOffsetFromSubAllocationStart()
			};
		}

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, const ShaderResourceView<Format, ViewDimension>& srv)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::CreateShaderResourceView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			mDescriptorInfoArr[index] = SRVInfo{
				.GPUResourcePtr{ &(srv.GetGPUResource()) },
				.SRVDesc{ srv.CreateSRVDescription() }
			};
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, const UnorderedAccessView<Format, ViewDimension>& uav)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::CreateUnorderedAccessView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			mDescriptorInfoArr[index] = UAVInfo{
				.GPUResourcePtr{ &(uav.GetGPUResource()) },
				.UAVCounter{ uav.GetUAVCounter() },
				.UAVDesc{ uav.CreateUAVDescription() }
			};
		}
	}
}