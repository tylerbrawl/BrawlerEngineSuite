module;
#include <limits>
#include "DxDef.h"

module Brawler.PSOBuilderCreators;
import Brawler.InputLayoutFieldResolver;
import Brawler.PerVertexIASlotDescription;
import Brawler.ShaderCompilationFlags;

namespace Brawler
{
	namespace PSOs
	{
		template <>
		PSOBuilder<PSOID::DEFERRED_GEOMETRY_RASTER> CreatePSOBuilder<PSOID::DEFERRED_GEOMETRY_RASTER>()
		{
			// TODO: There has to be an easier way to do this...

			PSOBuilder<PSOID::DEFERRED_GEOMETRY_RASTER> psoBuilder{};
			PSOStreamType<PSOID::DEFERRED_GEOMETRY_RASTER> psoStream{};

			{
				PSOShaderFieldResolver<CD3DX12_PIPELINE_STATE_STREAM_VS> vertexShaderResolver{ ShaderCompilationParams{
					.FilePath{ L"Shaders\\DeferredGeometryRasterVS.hlsl" },
					.EntryPoint{ L"main" },

					// We'll be using bindless SRVs.
					.CompilationFlags = ShaderCompilationFlags::RESOURCES_MAY_ALIAS
				} };

				psoBuilder.AddPSOFieldResolver(std::move(vertexShaderResolver));
			}

			{
				PSOShaderFieldResolver<CD3DX12_PIPELINE_STATE_STREAM_PS> pixelShaderResolver{ ShaderCompilationParams{
					.FilePath{ L"Shaders\\DeferredGeometryRasterPS.hlsl" },
					.EntryPoint{ L"main" },

					// We'll be using bindless SRVs.
					.CompilationFlags = ShaderCompilationFlags::RESOURCES_MAY_ALIAS
				} };

				psoBuilder.AddPSOFieldResolver(std::move(pixelShaderResolver));
			}

			{
				CD3DX12_BLEND_DESC& blendDesc{ static_cast<CD3DX12_BLEND_DESC&>(psoStream.BlendState) };
				blendDesc.AlphaToCoverageEnable = FALSE;

				// For G-buffer rendering with opaque objects, we always overwrite
				// values without performing blending for all render targets.
				blendDesc.IndependentBlendEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = FALSE;
			}

			psoStream.SampleMask = std::numeric_limits<std::uint32_t>::max();

			{
				CD3DX12_RASTERIZER_DESC& rasterizerDesc{ static_cast<CD3DX12_RASTERIZER_DESC&>(psoStream.RasterizerState) };
				rasterizerDesc.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;

				// This PSO is for opaque geometry.
				rasterizerDesc.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;

				// The Brawler Engine uses a left-handed coordinate system.
				rasterizerDesc.FrontCounterClockwise = FALSE;

				rasterizerDesc.DepthClipEnable = TRUE;
			}

			{
				CD3DX12_DEPTH_STENCIL_DESC1& depthStencilDesc{ static_cast<CD3DX12_DEPTH_STENCIL_DESC1&>(psoStream.DepthStencilState) };
				depthStencilDesc.DepthEnable = TRUE;
				depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;

				// We use reverse-Z depth buffers, so we only want to pass the depth test if
				// the value returned by the pixel shader is greater than that already present
				// in the depth buffer.
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;

				depthStencilDesc.StencilEnable = FALSE;
			}

			{
				InputLayoutFieldResolver inputLayoutFieldResolver{};

				/*
				Slot 0: PerVertexInputElement
				{
					DXGI_FORMAT_R32_UINT GLOBALVBINDEX
				};
				*/
				{
					PerVertexIASlotDescription slot0Description{};
					slot0Description.AddPerVertexInputElement("GLOBALVBINDEX", DXGI_FORMAT::DXGI_FORMAT_R32_UINT);

					inputLayoutFieldResolver.SetInputSlotData<0>(slot0Description);
				}

				/*
				Slot 1: PerVertexInputElement
				{
					DXGI_FORMAT_R32_UINT DESCRIPTORBUFFERINDICES;
				};
				*/
				{
					PerVertexIASlotDescription slot1Description{};
					slot1Description.AddPerVertexInputElement("DESCRIPTORBUFFERINDICES", DXGI_FORMAT::DXGI_FORMAT_R32_UINT);

					inputLayoutFieldResolver.SetInputSlotData<1>(slot1Description);
				}

				psoBuilder.AddPSOFieldResolver(std::move(inputLayoutFieldResolver));
			}

			psoStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			{
				D3D12_RT_FORMAT_ARRAY& renderTargetFormatArr{ static_cast<D3D12_RT_FORMAT_ARRAY&>(psoStream.RenderTargetFormats) };

				/*
				Our G-Buffer layout is defined as follows:

				SV_Target0: DXGI_FORMAT_R8G8B8A8_UNORM
				{
					R8: Base Color Red Channel
					G8: Base Color Green Channel
					B8: Base Color Blue Channel
					A8: GGX Roughness Squared
				};

				SV_Target1: DXGI_FORMAT_R8G8_UNORM
				{
					R8: Encoded World-Space Surface Normal X
					G8: Encoded World-Space Surface Normal Y
				};

				SV_Target2: DXGI_FORMAT_R8_UINT
				{
					R8: Metallic Indicator (0 = Dielectric, >0 = Metallic)
				};
				*/

				renderTargetFormatArr.NumRenderTargets = 3;
				renderTargetFormatArr.RTFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
				renderTargetFormatArr.RTFormats[1] = DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
				renderTargetFormatArr.RTFormats[2] = DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
			}

			// Our depth buffer has the format DXGI_FORMAT_D32_FLOAT.
			psoStream.DepthStencilFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

			{
				DXGI_SAMPLE_DESC& sampleDesc{ static_cast<DXGI_SAMPLE_DESC&>(psoStream.SampleDesc) };
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;
			}

			psoBuilder.SetPSODefaultValue(std::move(psoStream));
			return psoBuilder;
		}

		template <>
		PSOBuilder<PSOID::MODEL_INSTANCE_FRUSTUM_CULL> CreatePSOBuilder<PSOID::MODEL_INSTANCE_FRUSTUM_CULL>()
		{
			return CreateGeneralComputePSOBuilder<PSOID::MODEL_INSTANCE_FRUSTUM_CULL>(ShaderCompilationParams{
				.FilePath{ L"Shaders\\ModelInstanceFrustumCull.hlsl" },
				.EntryPoint{ L"main" },

				// We'll be using bindless SRVs.
				.CompilationFlags = ShaderCompilationFlags::RESOURCES_MAY_ALIAS
			});
		}

		template <>
		PSOBuilder<PSOID::DEFERRED_OPAQUE_SHADE> CreatePSOBuilder<PSOID::DEFERRED_OPAQUE_SHADE>()
		{
			return CreateGeneralComputePSOBuilder<PSOID::DEFERRED_OPAQUE_SHADE>(ShaderCompilationParams{
				.FilePath{ L"Shaders\\DeferredOpaqueShade.hlsl" },
				.EntryPoint{ L"main" },

				// We'll be using bindless SRVs.
				.CompilationFlags = ShaderCompilationFlags::RESOURCES_MAY_ALIAS
			});
		}
	}
}