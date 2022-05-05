module;
#include <cassert>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceBinder;
import Brawler.D3D12.PipelineEnums;
import Brawler.D3D12.GPUResourceHandle;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceDescriptors;

export namespace Brawler
{
	namespace D3D12
	{
		class DirectContext;
		class ComputeContext;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
		class GPUResourceBinder
		{
		private:
			friend class DirectContext;
			friend class ComputeContext;

		private:
			explicit GPUResourceBinder(Brawler::D3D12GraphicsCommandList& cmdList);

		public:
			GPUResourceBinder(const GPUResourceBinder& rhs) = delete;
			GPUResourceBinder& operator=(const GPUResourceBinder& rhs) = delete;

			GPUResourceBinder(GPUResourceBinder&& rhs) noexcept = default;
			GPUResourceBinder& operator=(GPUResourceBinder&& rhs) noexcept = default;

			template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::CBV)
			void BindRootCBV(const GPUResourceReadHandle hResource);

			template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::SRV)
			void BindRootSRV(const GPUResourceReadHandle hResource);

			template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::UAV)
			void BindRootUAV(const GPUResourceWriteHandle hResource);

			template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam, typename ConstantT>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::ROOT_CONSTANT) && (sizeof(ConstantT) % 4 == 0)
			void BindRoot32BitConstants(const ConstantT& rootConstants);

			template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE)
			void BindDescriptorTable(const I_DescriptorTable& descriptorTable);

		private:
			Brawler::D3D12GraphicsCommandList* mCmdList;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
		GPUResourceBinder<PSOIdentifier>::GPUResourceBinder(Brawler::D3D12GraphicsCommandList& cmdList) :
			mCmdList(&cmdList)
		{}

		template <Brawler::PSOs::PSOID PSOIdentifier>
		template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::CBV)
		void GPUResourceBinder<PSOIdentifier>::BindRootCBV(const GPUResourceReadHandle hResource)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootConstantBufferView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), hResource->GetD3D12Resource().GetGPUVirtualAddress());
			else
				mCmdList->SetComputeRootConstantBufferView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), hResource->GetD3D12Resource().GetGPUVirtualAddress());
		}

		template <Brawler::PSOs::PSOID PSOIdentifier>
		template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::SRV)
		void GPUResourceBinder<PSOIdentifier>::BindRootSRV(const GPUResourceReadHandle hResource)
		{
			assert(hResource->GetD3D12Resource().GetGPUVirtualAddress() != 0 && "ERROR: Only buffers can be root descriptors! Textures *MUST* be sent through a descriptor table!");

			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootShaderResourceView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), hResource->GetD3D12Resource().GetGPUVirtualAddress());
			else
				mCmdList->SetComputeRootShaderResourceView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), hResource->GetD3D12Resource().GetGPUVirtualAddress());
		}

		template <Brawler::PSOs::PSOID PSOIdentifier>
		template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::UAV)
		void GPUResourceBinder<PSOIdentifier>::BindRootUAV(const GPUResourceWriteHandle hResource)
		{
			assert(hResource->GetD3D12Resource().GetGPUVirtualAddress() != 0 && "ERROR: Only buffers can be root descriptors! Textures *MUST* be sent through a descriptor table!");

			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootUnorderedAccessView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), hResource->GetD3D12Resource().GetGPUVirtualAddress());
			else
				mCmdList->SetComputeRootUnorderedAccessView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), hResource->GetD3D12Resource().GetGPUVirtualAddress());
		}

		template <Brawler::PSOs::PSOID PSOIdentifier>
		template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam, typename ConstantT>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::ROOT_CONSTANT) && (sizeof(ConstantT) % 4 == 0)
		void GPUResourceBinder<PSOIdentifier>::BindRoot32BitConstants(const ConstantT& rootConstants)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRoot32BitConstants(static_cast<std::uint32_t>(std::to_underlying(RootParam)), (sizeof(ConstantT) / 4), &rootConstants, 0);
			else
				mCmdList->SetComputeRoot32BitConstants(static_cast<std::uint32_t>(std::to_underlying(RootParam)), (sizeof(ConstantT) / 4), &rootConstants, 0);
		}

		template <Brawler::PSOs::PSOID PSOIdentifier>
		template <Brawler::RootSignatures::RootParamEnumType<Brawler::PSOs::GetRootSignature<PSOIdentifier>()> RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE)
		void GPUResourceBinder<PSOIdentifier>::BindDescriptorTable(const I_DescriptorTable& descriptorTable)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootDescriptorTable(static_cast<std::uint32_t>(std::to_underlying(RootParam)), descriptorTable.GetDescriptorHandleInfo().HGPUDescriptor);
			else
				mCmdList->SetComputeRootDescriptorTable(static_cast<std::uint32_t>(std::to_underlying(RootParam)), descriptorTable.GetDescriptorHandleInfo().HGPUDescriptor);
		}
	}
}