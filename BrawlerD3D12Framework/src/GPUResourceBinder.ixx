module;
#include <cassert>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceBinder;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.I_DescriptorTable;
import Brawler.D3D12.RootDescriptors;
import Brawler.D3D12.RootParameterType;
import Brawler.D3D12.PipelineType;

export namespace Brawler
{
	namespace D3D12
	{
		class DirectContext;
		class ComputeContext;
	}
}

namespace Brawler
{
	namespace RootSignatures
	{
		template <auto RSIdentifier, auto RootParam>
		extern consteval auto GetRootParameterType();
	}

	namespace PSOs
	{
		template <auto PSOIdentifier>
		extern consteval auto GetRootSignature();

		template <auto PSOIdentifier>
		extern consteval auto GetPipelineType();
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <auto PSOIdentifier>
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

			template <auto RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::CBV)
			void BindRootCBV(const RootConstantBufferView rootCBV);

			template <auto RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::SRV)
			void BindRootSRV(const RootShaderResourceView rootSRV);

			template <auto RootParam>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::UAV)
			void BindRootUAV(const RootUnorderedAccessView rootUAV);

			template <auto RootParam, typename ConstantT>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::ROOT_CONSTANT) && (sizeof(std::remove_reference_t<ConstantT>) % 4 == 0) && (sizeof(std::remove_reference_t<ConstantT>) <= std::size_t)
			void BindRoot32BitConstants(const std::remove_reference_t<ConstantT> rootConstants);

			template <auto RootParam, typename ConstantT>
				requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::ROOT_CONSTANT) && (sizeof(ConstantT) % 4 == 0) && (sizeof(ConstantT) > std::size_t)
			void BindRoot32BitConstants(const ConstantT& rootConstants);

			template <auto RootParam>
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
		template <auto PSOIdentifier>
		GPUResourceBinder<PSOIdentifier>::GPUResourceBinder(Brawler::D3D12GraphicsCommandList& cmdList) :
			mCmdList(&cmdList)
		{}

		template <auto PSOIdentifier>
		template <auto RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::CBV)
		void GPUResourceBinder<PSOIdentifier>::BindRootCBV(const RootConstantBufferView rootCBV)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootConstantBufferView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), rootCBV.GetGPUVirtualAddress());
			else
				mCmdList->SetComputeRootConstantBufferView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), rootCBV.GetGPUVirtualAddress());
		}

		template <auto PSOIdentifier>
		template <auto RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::SRV)
		void GPUResourceBinder<PSOIdentifier>::BindRootSRV(const RootShaderResourceView rootSRV)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootShaderResourceView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), rootSRV.GetGPUVirtualAddress());
			else
				mCmdList->SetComputeRootShaderResourceView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), rootSRV.GetGPUVirtualAddress());
		}

		template <auto PSOIdentifier>
		template <auto RootParam>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::UAV)
		void GPUResourceBinder<PSOIdentifier>::BindRootUAV(const RootUnorderedAccessView rootUAV)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRootUnorderedAccessView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), rootUAV.GetGPUVirtualAddress());
			else
				mCmdList->SetComputeRootUnorderedAccessView(static_cast<std::uint32_t>(std::to_underlying(RootParam)), rootUAV.GetGPUVirtualAddress());
		}

		template <auto PSOIdentifier>
		template <auto RootParam, typename ConstantT>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::ROOT_CONSTANT) && (sizeof(std::remove_reference_t<ConstantT>) % 4 == 0) && (sizeof(std::remove_reference_t<ConstantT>) <= std::size_t)
		void GPUResourceBinder<PSOIdentifier>::BindRoot32BitConstants(const std::remove_reference_t<ConstantT> rootConstants)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRoot32BitConstants(static_cast<std::uint32_t>(std::to_underlying(RootParam)), (sizeof(rootConstants) / 4), &rootConstants, 0);
			else
				mCmdList->SetComputeRoot32BitConstants(static_cast<std::uint32_t>(std::to_underlying(RootParam)), (sizeof(rootConstants) / 4), &rootConstants, 0);
		}

		template <auto PSOIdentifier>
		template <auto RootParam, typename ConstantT>
			requires (Brawler::RootSignatures::GetRootParameterType<Brawler::PSOs::GetRootSignature<PSOIdentifier>(), RootParam>() == Brawler::RootParameters::RootParameterType::ROOT_CONSTANT) && (sizeof(ConstantT) % 4 == 0) && (sizeof(ConstantT) > std::size_t)
		void GPUResourceBinder<PSOIdentifier>::BindRoot32BitConstants(const ConstantT& rootConstants)
		{
			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				mCmdList->SetGraphicsRoot32BitConstants(static_cast<std::uint32_t>(std::to_underlying(RootParam)), (sizeof(rootConstants) / 4), &rootConstants, 0);
			else
				mCmdList->SetComputeRoot32BitConstants(static_cast<std::uint32_t>(std::to_underlying(RootParam)), (sizeof(rootConstants) / 4), &rootConstants, 0);
		}

		template <auto PSOIdentifier>
		template <auto RootParam>
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