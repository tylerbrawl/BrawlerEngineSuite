module;
#include <limits>
#include <array>
#include "DxDef.h"

export module Brawler.IMPL.RootSignatureDef;
import Brawler.PipelineEnums;
import Util.Engine;

namespace Brawler
{
	namespace IMPL
	{
		// This should be the same value as Brawler::IMPL::BINDLESS_SRV_SEGMENT_SIZE in
		// ResourceDescriptorHeap.ixx.
		static constexpr std::uint32_t BINDLESS_SRV_COUNT = 500000;

		template <std::uint32_t RegisterSpace>
		struct BindlessDescriptorRange
		{
			static constexpr D3D12_DESCRIPTOR_RANGE1 RANGE{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				.NumDescriptors = BINDLESS_SRV_COUNT,
				.BaseShaderRegister = 0,
				.RegisterSpace = RegisterSpace,
				.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
				.OffsetInDescriptorsFromTableStart = 0
			};
		};

		// There should be 1 descriptor range for every type of SRV which will be
		// used in the bindless descriptor table. For instance, there can be a
		// range for 2D textures, a range for 3D textures, and a range for
		// StructuredBuffers.
		static constexpr std::size_t BINDLESS_DESCRIPTOR_RANGE_COUNT = 1;

		static constexpr std::array<D3D12_DESCRIPTOR_RANGE1, BINDLESS_DESCRIPTOR_RANGE_COUNT> BINDLESS_DESCRIPTOR_RANGE_ARR{
			BindlessDescriptorRange<0>::RANGE
		};

		constexpr Brawler::D3D12_ROOT_PARAMETER CreateBindlessDescriptorTableRootParameter()
		{
			// I hope I am understanding this right, because the MSDN's documentation for
			// bindless descriptor tables SUCKS. Anyways, we are going to use register t0
			// to create a descriptor table containing overlapping ranges of SRVs in separate
			// spaces within the same register.
			//
			// The idea is to create a descriptor range for every possible type of SRV
			// which can appear in the bindless descriptor table. This is how other people
			// seem to be doing it, anyways.

			return Brawler::D3D12_ROOT_PARAMETER{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1{
					.NumDescriptorRanges = BINDLESS_DESCRIPTOR_RANGE_COUNT,
					.pDescriptorRanges = BINDLESS_DESCRIPTOR_RANGE_ARR.data()
				},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
			};
		}

		template <Brawler::RootSignatureID ID>
		struct RootSignatureDefinition
		{
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: An attempt was made to get a root signature description for a given RootSignatureID, but no such description was ever defined!");

			/*
			using RootParamEnumType = [ ... ];

			static constexpr D3D12_VERSIONED_ROOT_SIGNATURE_DESC ROOT_SIGNATURE_DESCRIPTION{ ... };
			*/
		};
	}
}

export namespace Brawler
{
	namespace IMPL
	{
		template <Brawler::RootSignatureID ID>
		using RootParamEnumType = RootSignatureDefinition<ID>::RootParamEnumType;

		template <Brawler::RootSignatureID ID>
		constexpr D3D12_VERSIONED_ROOT_SIGNATURE_DESC GetRootSignatureDescription()
		{
			return RootSignatureDefinition<ID>::ROOT_SIGNATURE_DESCRIPTION;
		}

		template <Brawler::RootSignatureID ID>
		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetSerializedRootSignature()
		{
			constexpr D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{ GetRootSignatureDescription<ID>() };
			Microsoft::WRL::ComPtr<ID3DBlob> rootSigBlob{ nullptr };
			Microsoft::WRL::ComPtr<ID3DBlob> errorBlob{ nullptr };

			CheckHRESULT(D3DX12SerializeVersionedRootSignature(
				&rootSigDesc,
				D3D_ROOT_SIGNATURE_VERSION_1_1,
				&rootSigBlob,
				&errorBlob
			));

			if (errorBlob != nullptr) [[unlikely]]
			{
				const std::string_view errMsg{ reinterpret_cast<const char*>(errorBlob->GetBufferPointer()) };
				throw std::runtime_error{ "ERROR: Serializing a root signature failed with the following error: " + std::string{ errMsg } };
			}

			Microsoft::WRL::ComPtr<ID3D12RootSignature> serializedRootSig{ nullptr };
			CheckHRESULT(Util::Engine::GetD3D12Device().CreateRootSignature(
				0,
				rootSigBlob->GetBufferPointer(),
				rootSigBlob->GetBufferSize(),
				IID_PPV_ARGS(&serializedRootSig)
			));

			return serializedRootSig;
		}
	}
}