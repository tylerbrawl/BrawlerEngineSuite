module;
#include "DxDef.h"

export module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.PerFrameDescriptorTable;
import Brawler.OptionalRef;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.UAVCounterSubAllocation;

export namespace Brawler
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
			struct SRVInfo
			{
				Brawler::D3D12Resource& D3DResource;
				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			};

			struct UAVInfo
			{
				Brawler::D3D12Resource& D3DResource;

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
				Brawler::OptionalRef<D3D12Resource> UAVCounterD3DResource;

				D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			};

		public:
			explicit DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors);

			DescriptorTableBuilder(const DescriptorTableBuilder& rhs) = delete;
			DescriptorTableBuilder& operator=(const DescriptorTableBuilder& rhs) = delete;

			DescriptorTableBuilder(DescriptorTableBuilder&& rhs) noexcept = default;
			DescriptorTableBuilder& operator=(DescriptorTableBuilder&& rhs) noexcept = default;

			template <typename DataElementType>
			void CreateConstantBufferView(const std::uint32_t index, const ConstantBufferView<DataElementType> cbv);

			template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
			void CreateShaderResourceView(const std::uint32_t index, const ShaderResourceView<Format, ViewDimension>& srv);

			template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
			void CreateUnorderedAccessView(const std::uint32_t index, const UnorderedAccessView<Format, ViewDimension>& uav);

			PerFrameDescriptorTable FinalizeDescriptorTable() const;

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
			void CreateConstantBufferView(const std::uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc);
			void CreateShaderResourceView(const std::uint32_t index, const SRVInfo& srvInfo);
			void CreateUnorderedAccessView(const std::uint32_t index, const UAVInfo& uavInfo);

			CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mStagingHeap;
			std::uint32_t mNumDescriptors;
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
			CreateConstantBufferView(index, cbv.CreateCBVDescription());
		}

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, const ShaderResourceView<Format, ViewDimension>& srv)
		{
			CreateShaderResourceView(index, SRVInfo{
				.D3DResource{ srv.GetD3D12Resource() },
				.SRVDesc{ srv.CreateSRVDescription() }
			});
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, const UnorderedAccessView<Format, ViewDimension>& uav)
		{
			Brawler::OptionalRef<Brawler::D3D12Resource> uavCounterResource{};

			{
				const Brawler::OptionalRef<const UAVCounterSubAllocation> uavCounterSubAllocation{ uav.GetUAVCounter() };

				if (uavCounterSubAllocation.HasValue())
					uavCounterResource = uavCounterSubAllocation->GetD3D12Resource();
			}

			CreateUnorderedAccessView(index, UAVInfo{
				.D3DResource{uav.GetD3D12Resource()},
				.UAVCounterD3DResource{std::move(uavCounterResource)},
				.UAVDesc{uav.CreateUAVDescription()}
			});
		}
	}
}