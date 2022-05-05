module;
#include <type_traits>
#include "DxDef.h"

export module Brawler.GraphicsRootParameterBinder;
import Brawler.PipelineEnums;
import Brawler.IMPL.RootParamDef;
import Brawler.IMPL.PSODef;
import Brawler.IMPL.RootSignatureDef;
import Brawler.I_GPUResource;
import Brawler.GPUResourceHandle;
import Brawler.ResourceDescriptorTable;
import Brawler.DescriptorHandleInfo;

namespace Brawler
{
	namespace IMPL
	{
		template <RootParamType ParamType>
		struct ResourceBinder
		{
			template <typename RootParamEnumType, RootParamEnumType RootParamID>
			__forceinline static void BindResource(Brawler::D3D12GraphicsCommandList& cmdList, RootParameterBindingType<RootParamType, ParamType> resource);
		};
	}
}

export namespace Brawler
{
	template <Brawler::PSOID PipelineStateObjectID>
	class GraphicsRootParameterBinder
	{
	private:
		using RootParamEnumType = Brawler::IMPL::RootParamEnumType<Brawler::IMPL::GetPSORootSignatureID<PipelineStateObjectID>()>;
		
		template <RootParamEnumType RootParamID>
		using ResourceBindingType = Brawler::IMPL::RootParameterBindingType<std::underlying_type_t<Brawler::IMPL::RootParamType>, Brawler::IMPL::GetRootParameterType<RootParamEnumType, RootParamID>()>;

	public:
		explicit GraphicsRootParameterBinder(Brawler::D3D12GraphicsCommandList& cmdList);

		GraphicsRootParameterBinder(const GraphicsRootParameterBinder& rhs) = delete;
		GraphicsRootParameterBinder& operator=(const GraphicsRootParameterBinder& rhs) = delete;

		GraphicsRootParameterBinder(GraphicsRootParameterBinder&& rhs) noexcept = default;
		GraphicsRootParameterBinder& operator=(GraphicsRootParameterBinder&& rhs) noexcept = default;

		template <RootParamEnumType RootParamID>
		void BindResource(ResourceBindingType<RootParamID> resource);

	private:
		Brawler::D3D12GraphicsCommandList* mCmdList;
	};
}

// --------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace IMPL
	{
		template <>
		template <typename RootParamEnumType, RootParamEnumType RootParamID>
		__forceinline void ResourceBinder<RootParamType::UINT32_CONSTANT>::BindResource(Brawler::D3D12GraphicsCommandList& cmdList, RootParameterBindingType<RootParamType, RootParamType::UINT32_CONSTANT> resource)
		{
			constexpr std::uint32_t ROOT_PARAM_INDEX = GetRootParameterIndex<RootParamEnumType, RootParamID>();
			cmdList.SetGraphicsRoot32BitConstant(ROOT_PARAM_INDEX, resource, 0);
		}

		template <>
		template <typename RootParamEnumType, RootParamEnumType RootParamID>
		__forceinline void ResourceBinder<RootParamType::CBV>::BindResource(Brawler::D3D12GraphicsCommandList& cmdList, RootParameterBindingType<RootParamType, RootParamType::CBV> resource)
		{
			constexpr std::uint32_t ROOT_PARAM_INDEX = GetRootParameterIndex<RootParamEnumType, RootParamID>();
			cmdList.SetGraphicsRootConstantBufferView(ROOT_PARAM_INDEX, resource->GetD3D12Resource().GetGPUVirtualAddress());
		}

		template <>
		template <typename RootParamEnumType, RootParamEnumType RootParamID>
		__forceinline void ResourceBinder<RootParamType::SRV>::BindResource(Brawler::D3D12GraphicsCommandList& cmdList, RootParameterBindingType<RootParamType, RootParamType::SRV> resource)
		{
			constexpr std::uint32_t ROOT_PARAM_INDEX = GetRootParameterIndex<RootParamEnumType, RootParamID>();
			cmdList.SetGraphicsRootShaderResourceView(ROOT_PARAM_INDEX, resource->GetD3D12Resource().GetGPUVirtualAddress());
		}

		template <>
		template <typename RootParamEnumType, RootParamEnumType RootParamID>
		__forceinline void ResourceBinder<RootParamType::UAV>::BindResource(Brawler::D3D12GraphicsCommandList& cmdList, RootParameterBindingType<RootParamType, RootParamType::UAV> resource)
		{
			constexpr std::uint32_t ROOT_PARAM_INDEX = GetRootParameterIndex<RootParamEnumType, RootParamID>();
			cmdList.SetGraphicsRootUnorderedAccessView(ROOT_PARAM_INDEX, resource->GetD3D12Resource().GetGPUVirtualAddress());
		}

		template <>
		template <typename RootParamEnumType, RootParamEnumType RootParamID>
		__forceinline void ResourceBinder<RootParamType::DESCRIPTOR_TABLE>::BindResource(Brawler::D3D12GraphicsCommandList& cmdList, RootParameterBindingType<RootParamType, RootParamType::DESCRIPTOR_TABLE> resource)
		{
			constexpr std::uint32_t ROOT_PARAM_INDEX = GetRootParameterIndex<RootParamEnumType, RootParamID>();

			// Ensure that the relevant descriptors are copied to the ResourceDescriptorHeap.
			resource.CreatePerFrameDescriptorTable();

			const Brawler::DescriptorHandleInfo& descriptorTableHandles{ resource.GetDescriptorHandlesForTableStart() };
			cmdList.SetGraphicsRootDescriptorTable(ROOT_PARAM_INDEX, descriptorTableHandles.GPUHandle);
		}
	}

	template <Brawler::PSOID PipelineStateObjectID>
	GraphicsRootParameterBinder<PipelineStateObjectID>::GraphicsRootParameterBinder(Brawler::D3D12GraphicsCommandList& cmdList) :
		mCmdList(&cmdList)
	{}

	template <Brawler::PSOID PipelineStateObjectID>
	template <GraphicsRootParameterBinder<PipelineStateObjectID>::RootParamEnumType RootParamID>
	void GraphicsRootParameterBinder<PipelineStateObjectID>::BindResource(ResourceBindingType<RootParamID> resource)
	{
		constexpr Brawler::IMPL::RootParamType ROOT_PARAM_TYPE = Brawler::IMPL::GetRootParameterType<RootParamEnumType, RootParamID>();
		
		Brawler::IMPL::ResourceBinder<ROOT_PARAM_TYPE>::template BindResource<RootParamEnumType, RootParamID>(*mCmdList, resource);
	}
}