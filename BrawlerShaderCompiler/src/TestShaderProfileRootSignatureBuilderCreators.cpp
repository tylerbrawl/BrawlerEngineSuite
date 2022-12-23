module;
#include <vector>
#include "DxDef.h"

module Brawler.RootSignatureBuilderCreators;

namespace Brawler
{
	namespace RootSignatures
	{
		template <>
		RootSignatureBuilder<Brawler::RootSignatureID::TEST_ROOT_SIGNATURE> CreateRootSignatureBuilder<Brawler::RootSignatureID::TEST_ROOT_SIGNATURE>()
		{
			RootSignatureBuilder<RootSignatureID::TEST_ROOT_SIGNATURE> rootSigBuilder{};

			// Root Parameter 0: ConstantsBuffer<3> -> Space0[b2]
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_0>(RootConstantsInfo{
					.Num32BitValues = 3,
					.ShaderRegister = 2,
					.RegisterSpace = 0,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			/*
			Root Parameter 1: DescriptorTable
			{
				SRV[6] -> Space0[t2 - t7]
				UAV[4] -> Space0[u0 - u3]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param1TableRanges{};
				param1TableRanges.resize(2);

				param1TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 2);
				param1TableRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_1>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param1TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			// Root Parameter 2: STATIC CBV -> Space0[b0]
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_2>(RootCBVInfo{
					.ShaderRegister = 0,
					.RegisterSpace = 0,
					.Flags = D3D12_ROOT_DESCRIPTOR_FLAGS::D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			/*
			Root Parameter 3: DescriptorTable
			{
				SAMPLER[2] -> Space0[s0 - s1]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param3TableRanges{};
				param3TableRanges.resize(1);

				param3TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_3>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param3TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			/*
			Root Parameter 4: DescriptorTable
			{
				VOLATILE SRV[?] -> Space0[t8 - ...]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param4TableRanges{};
				param4TableRanges.resize(1);

				param4TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, std::numeric_limits<std::uint32_t>::max(), 8, 0);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_4>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param4TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			/*
			Root Parameter 5: DescriptorTable
			{
				VOLATILE SRV[?] -> Space1[t0 - ...]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param5TableRanges{};
				param5TableRanges.resize(1);

				param5TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, std::numeric_limits<std::uint32_t>::max(), 0, 1);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_5>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param5TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			/*
			Root Parameter 6: DescriptorTable
			{
				STATIC CBV[1] -> Space0[b1]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param6TableRanges{};
				param6TableRanges.resize(1);

				param6TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::TestRootSignature::PARAM_6>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param6TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
					});
			}

			// Static Sampler 0: Anisotropic Sampler -> s3
			{
				CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler{};
				anisotropicSampler.Init(3, D3D12_FILTER::D3D12_FILTER_ANISOTROPIC);

				rootSigBuilder.AddStaticSampler(std::move(anisotropicSampler));
			}

			return rootSigBuilder;
		}
	}
}