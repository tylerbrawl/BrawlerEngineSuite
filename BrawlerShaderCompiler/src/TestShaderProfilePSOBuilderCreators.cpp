module;
#include "DxDef.h"

module Brawler.PSOBuilderCreators;
import Brawler.InputLayoutFieldResolver;
import Brawler.PerVertexIASlotDescription;

namespace Brawler
{
	namespace PSOs
	{
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

			{
				PerVertexIASlotDescription slot0Description{};
				slot0Description.AddPerVertexInputElement("POSITION", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
				slot0Description.AddPerVertexInputElement("NORMAL", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
				slot0Description.AddPerVertexInputElement("UV", DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT);
				slot0Description.AddPerVertexInputElement("TANGENT", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);
				slot0Description.AddPerVertexInputElement("BITANGENT", DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT);

				inputLayoutResolver.SetInputSlotData<0>(slot0Description);
			}

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