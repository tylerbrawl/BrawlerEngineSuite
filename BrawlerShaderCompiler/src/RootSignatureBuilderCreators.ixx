module;
#include "DxDef.h"

export module Brawler.RootSignatureBuilderCreators;
import Brawler.RootSignatureID;
import Brawler.RootSignatureBuilder;

export namespace Brawler
{
	namespace RootSignatures
	{
		template <Brawler::RootSignatureID RSIdentifier>
		RootSignatureBuilder<RSIdentifier> CreateRootSignatureBuilder()
		{
			static_assert(sizeof(RSIdentifier) != sizeof(RSIdentifier), "ERROR: An explicit template specialization of Brawler::RootSignatures::CreateRootSignatureBuilder() was never provided for a particular RootSignatureID! (See RootSignatureBuilderCreators.ixx.)");
		}

		template <>
		RootSignatureBuilder<Brawler::RootSignatureID::BC6H_BC7_COMPRESSION> CreateRootSignatureBuilder<Brawler::RootSignatureID::BC6H_BC7_COMPRESSION>()
		{
			RootSignatureBuilder<Brawler::RootSignatureID::BC6H_BC7_COMPRESSION> rootSigBuilder{};

			/*
			Root Parameter 0: DescriptorTable
			{
				STATIC SRV[1] -> Space0[t0]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param0TableRanges{};
				param0TableRanges.resize(1);

				// Static Data: The source texture is initialized before it is ever bound to the root signature, and it is never
				// written to.
				param0TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::BC6HBC7Compression::SOURCE_TEXTURE_SRV_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{ std::move(param0TableRanges) },
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 1: DescriptorTable
			{
				STATIC_AT_EXECUTE SRV[1] -> Space0[t1]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param1TableRanges{};
				param1TableRanges.resize(1);

				// Static-at-Execute Data: We promise that the data of the input buffer is static after it has been bound to the
				// root signature. However, since this input buffer will likely be used as the output buffer for the next iteration,
				// it cannot be completely static.
				param1TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::BC6HBC7Compression::INPUT_BUFFER_SRV_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{ std::move(param1TableRanges) },
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 2: DescriptorTable
			{
				VOLATILE UAV[1] -> Space0[u0]
			}
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param2TableRanges{};
				param2TableRanges.resize(1);

				param2TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::BC6HBC7Compression::OUTPUT_BUFFER_UAV_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{ std::move(param2TableRanges) },
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			// Root Parameter 3: STATIC CBV -> Space0[b0]
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::BC6HBC7Compression::COMPRESSION_SETTINGS_CBV>(RootCBVInfo{
					.ShaderRegister = 0,
					.RegisterSpace = 0,

					// Static Data: The compression settings constant buffers are all initialized prior to command list
					// recording.
					.Flags = D3D12_ROOT_DESCRIPTOR_FLAGS::D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,

					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 4: RootConstantsBuffer<2> -> Space0[b1]
			*/
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::BC6HBC7Compression::MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS>(RootConstantsInfo{
					.Num32BitValues = 2,
					.ShaderRegister = 1,
					.RegisterSpace = 0,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			return rootSigBuilder;
		}

		template <>
		RootSignatureBuilder<Brawler::RootSignatureID::GENERIC_DOWNSAMPLE> CreateRootSignatureBuilder<Brawler::RootSignatureID::GENERIC_DOWNSAMPLE>()
		{
			RootSignatureBuilder<RootSignatureID::GENERIC_DOWNSAMPLE> rootSigBuilder{};

			/*
			Root Parameter 0: DescriptorTable
			{
				STATIC_AT_EXECUTE SRV InputTexture -> Space0[t0];
				VOLATILE UAV OutputTexture1 -> Space0[u0];
				VOLATILE UAV OutputTexture2 -> Space0[u1];
			};
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param0TableRanges{};
				param0TableRanges.resize(2);

				// Static-at-Execute Data: When the descriptor table is bound on the GPU timeline, we guarantee that InputTexture will not be
				// written to. However, we cannot set this as completely static, because this texture may have previously been used as an
				// output texture. This is common for mip-mapping.
				param0TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

				param0TableRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::GenericDownsample::TEXTURES_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{ std::move(param0TableRanges) },
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			// Root Parameter 1: RootConstants<4> -> Space0[b0]
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::GenericDownsample::MIP_MAP_CONSTANTS>(RootConstantsInfo{
					.Num32BitValues = 4,
					.ShaderRegister = 0,
					.RegisterSpace = 0,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			// Static Sampler 0: Bilinear Clamp
			{
				CD3DX12_STATIC_SAMPLER_DESC staticBilinearClampSampler{};
				staticBilinearClampSampler.Init(
					0,
					D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					0,
					1,
					D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER,
					D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
					0.0f,
					D3D12_FLOAT32_MAX,
					D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL,
					0
				);

				rootSigBuilder.AddStaticSampler(std::move(staticBilinearClampSampler));
			}

			return rootSigBuilder;
		}

		template <>
		RootSignatureBuilder<Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_TILING> CreateRootSignatureBuilder<Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_TILING>()
		{
			RootSignatureBuilder<Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_TILING> rootSigBuilder{};

			// Root Parameter 0: RootConstants<3> TilingConstants -> Space0[b0];
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageTiling::TILING_ROOT_CONSTANTS>(RootConstantsInfo{
					.Num32BitValues = 3,
					.ShaderRegister = 0,
					.RegisterSpace = 0,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 1: Descriptor Table
			{
				VOLATILE UAV OutputPage0 -> Space0[u0];
				VOLATILE UAV OutputPage1 -> Space0[u1];
				VOLATILE UAV OutputPage2 -> Space0[u2];
				VOLATILE UAV OutputPage3 -> Space0[u3];
			};
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param1TableRanges{};
				param1TableRanges.resize(1);

				param1TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageTiling::OUTPUT_PAGES_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param1TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			// Root Parameter 2: STATIC CBV MipLevelConstants -> Space0[b1];
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageTiling::MIP_LEVEL_INFO_CBV>(RootCBVInfo{
					.ShaderRegister = 1,
					.RegisterSpace = 0,

					// Static Data: The constant buffers describing each mip level are initialized before any of them are
					// bound as a root parameter.
					.Flags = D3D12_ROOT_DESCRIPTOR_FLAGS::D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC,

					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 3: Descriptor Table
			{
				STATIC SRV InputTexture -> Space0[t0];
			};
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param3TableRanges{};
				param3TableRanges.resize(1);

				// Static Data: At this point, all of the input texture's mip levels have been generated, and we do not expect to modify
				// its data any further.
				param3TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageTiling::INPUT_TEXTURE_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param3TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			// Static Sampler 0: Point Clamp
			{
				CD3DX12_STATIC_SAMPLER_DESC staticPointClampSampler{};
				staticPointClampSampler.Init(
					0,
					D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					0.0f,
					1,
					D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER,
					D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
					0.0f,
					D3D12_FLOAT32_MAX,
					D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL,
					0
				);

				rootSigBuilder.AddStaticSampler(std::move(staticPointClampSampler));
			}

			return rootSigBuilder;
		}

		template <>
		RootSignatureBuilder<Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_MERGING> CreateRootSignatureBuilder<Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_MERGING>()
		{
			RootSignatureBuilder<RootSignatureID::VIRTUAL_TEXTURE_PAGE_MERGING> rootSigBuilder{};

			// Root Parameter 0: RootConstants<3> TilingConstants->Space0[b0];
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageMerging::TILING_CONSTANTS>(RootConstantsInfo{
					.Num32BitValues = 3,
					.ShaderRegister = 0,
					.RegisterSpace = 0,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 1: DescriptorTable
			{
				VOLATILE UAV OutputTexture -> Space0[u0];
			};
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param1TableRanges{};
				param1TableRanges.resize(1);

				param1TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageMerging::OUTPUT_TEXTURE_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param1TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			/*
			Root Parameter 2: DescriptorTable
			{
				STATIC SRV InputTexture -> Space0[t0];
			};
			*/
			{
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> param2TableRanges{};
				param2TableRanges.resize(1);

				// Static Data: At this point, all of the input texture's mip levels have been generated, and we do not expect to modify
				// its data any further.
				param2TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::VirtualTexturePageMerging::INPUT_TEXTURE_TABLE>(DescriptorTableInfo{
					.DescriptorRangeArr{std::move(param2TableRanges)},
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL
				});
			}

			// Static Sampler 0: Point Clamp
			{
				CD3DX12_STATIC_SAMPLER_DESC staticPointClampSampler{};
				staticPointClampSampler.Init(
					0,
					D3D12_FILTER::D3D12_FILTER_MIN_MAG_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					0.0f,
					1,
					D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER,
					D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
					0.0f,
					D3D12_FLOAT32_MAX,
					D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL,
					0
				);

				rootSigBuilder.AddStaticSampler(std::move(staticPointClampSampler));
			}

			return rootSigBuilder;
		}

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