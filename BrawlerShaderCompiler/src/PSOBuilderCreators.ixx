module;
#include <functional>
#include "DxDef.h"

export module Brawler.PSOBuilderCreators;
import Brawler.AppParams;  // Classic MSVC modules jank...
import Brawler.PSOID;
import Brawler.PSOBuilder;
import Brawler.InputLayoutFieldResolver;
import Brawler.PSOShaderFieldResolver;
import Brawler.ShaderCompilationParams;

/*
For information regarding some of the fields of a typical PSO, read the following MSDN articles *CAREFULLY*:

https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage

*/

namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		PSOBuilder<PSOIdentifier> CreateGeneralComputePSOBuilder(Brawler::ShaderCompilationParams&& compileParams)
		{
			Brawler::PSOShaderFieldResolver<CD3DX12_PIPELINE_STATE_STREAM_CS> computeShaderResolver{ std::move(compileParams) };

			PSOBuilder<PSOIdentifier> psoBuilder{};
			psoBuilder.AddPSOFieldResolver(std::move(computeShaderResolver));

			return psoBuilder;
		}
	}
}

export namespace Brawler
{
	namespace PSOs
	{
		template <Brawler::PSOID PSOIdentifier>
		PSOBuilder<PSOIdentifier> CreatePSOBuilder()
		{
			static_assert(sizeof(PSOIdentifier) != sizeof(PSOIdentifier), "ERROR: An explicit template specialization of Brawler::PSOs::CreatePSOBuilder() was never created for a particular PSOID!");
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_TRY_MODE_456> CreatePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_456>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_456>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"TryMode456CS" },
				.MacroDefinitionArr{}
			});
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_TRY_MODE_137> CreatePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_137>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_137>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"TryMode137CS" },
				.MacroDefinitionArr{}
			});
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_TRY_MODE_02> CreatePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_02>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_02>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"TryMode02CS" },
				.MacroDefinitionArr{}
			});
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_ENCODE_BLOCK> CreatePSOBuilder<Brawler::PSOID::BC7_ENCODE_BLOCK>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_ENCODE_BLOCK>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"EncodeBlockCS" },
				.MacroDefinitionArr{}
			});
		}

		template <>
		PSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE> CreatePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\GenerateMipMaps.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{}
			});
		}

		template <>
		PSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB> CreatePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\GenerateMipMaps.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					// For tools like the Brawler Model Exporter, we might as well just use the exact sRGB
					// curve.
					{ L"__EXACT_SRGB__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" }
				}
			});
		}

		template <>
		PSOBuilder<Brawler::PSOID::TEST_PSO> CreatePSOBuilder<Brawler::PSOID::TEST_PSO>()
		{
			Brawler::PSOStreamType<Brawler::PSOID::TEST_PSO> psoStream{};

			Brawler::PSOShaderFieldResolver<CD3DX12_PIPELINE_STATE_STREAM_VS> vertexShaderResolver{ Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\Mesh.hlsl" },
				.EntryPoint{ L"VS" },
				.MacroDefinitionArr{}
			} };

			Brawler::PSOShaderFieldResolver<CD3DX12_PIPELINE_STATE_STREAM_PS> pixelShaderResolver{ Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\Mesh.hlsl" },
				.EntryPoint{ L"PSGBuffer" },
				.MacroDefinitionArr{
					{ L"OutputUVGradients_", L"1" }
				}
			} };

			CD3DX12_BLEND_DESC& blendDesc{ static_cast<CD3DX12_BLEND_DESC&>(psoStream.BlendState) };
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;

			psoStream.SampleMask = 0xFFFFFFFF;

			CD3DX12_RASTERIZER_DESC& rasterizerState{ static_cast<CD3DX12_RASTERIZER_DESC&>(psoStream.RasterizerState) };
			rasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
			rasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
			rasterizerState.DepthClipEnable = true;
			rasterizerState.MultisampleEnable = true;

			// Use reverse-Z depth buffers.
			CD3DX12_DEPTH_STENCIL_DESC1& depthStencilState{ static_cast<CD3DX12_DEPTH_STENCIL_DESC1&>(psoStream.DepthStencilState) };
			depthStencilState.DepthEnable = true;
			depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
			depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
			depthStencilState.StencilEnable = false;

			Brawler::InputLayoutFieldResolver inputLayoutResolver{};
			inputLayoutResolver.AddPerVertexInputElement("POSITION", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
			inputLayoutResolver.AddPerVertexInputElement("NORMAL", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
			inputLayoutResolver.AddPerVertexInputElement("UV", DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT);
			inputLayoutResolver.AddPerVertexInputElement("TANGENT", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
			inputLayoutResolver.AddPerVertexInputElement("BITANGENT", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);

			psoStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			// We'll simulate having four G-buffers.
			D3D12_RT_FORMAT_ARRAY& renderTargetFormats{ static_cast<D3D12_RT_FORMAT_ARRAY&>(psoStream.RenderTargetFormats) };
			renderTargetFormats.NumRenderTargets = 4;
			renderTargetFormats.RTFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
			renderTargetFormats.RTFormats[1] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM;
			renderTargetFormats.RTFormats[2] = DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
			renderTargetFormats.RTFormats[3] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM;

			psoStream.DepthStencilFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

			DXGI_SAMPLE_DESC& sampleDesc{ static_cast<DXGI_SAMPLE_DESC&>(psoStream.SampleDesc) };
			sampleDesc.Count = 1;
			sampleDesc.Quality = 0;

			PSOBuilder<Brawler::PSOID::TEST_PSO> psoBuilder{};
			psoBuilder.SetPSODefaultValue(std::move(psoStream));

			psoBuilder.AddPSOFieldResolver(std::move(inputLayoutResolver));
			psoBuilder.AddPSOFieldResolver(std::move(vertexShaderResolver));
			psoBuilder.AddPSOFieldResolver(std::move(pixelShaderResolver));

			return psoBuilder;
		}
	}
}