module;
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.Texture2D;
import Brawler.D3D12.GPUResourceInitializationInfo;

namespace
{
	constexpr Brawler::D3D12_RESOURCE_DESC CreateTexture2DResourceDescription(const Brawler::D3D12::Texture2DInitializationInfo& initInfo)
	{
		return Brawler::D3D12_RESOURCE_DESC{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = initInfo.Width,
			.Height = initInfo.Height,
			.DepthOrArraySize = 1,
			.MipLevels = initInfo.MipLevels,
			.Format = initInfo.Format,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = initInfo.Layout,
			.Flags = initInfo.Flags,
			.SamplerFeedbackMipRegion{}
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		Texture2D::Texture2D(const Texture2DInitializationInfo& initInfo) :
			I_GPUResource(GPUResourceInitializationInfo{
			.ResourceDesc{ CreateTexture2DResourceDescription(initInfo) },
			.AllocationDesc{ initInfo.AllocationDesc },
			.InitialResourceState = initInfo.InitialResourceState
			})
		{}

		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> Texture2D::CreateSRVDescription() const
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
				.Format = GetResourceDescription().Format,
				.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D{
					.MostDetailedMip = 0,
					.MipLevels = std::numeric_limits<std::uint32_t>::max(),
					.PlaneSlice = 0,
					.ResourceMinLODClamp = 0.0f
				}
			};

			return std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>{ std::move(srvDesc) };
		}

		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> Texture2D::CreateUAVDescription() const
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
				.Format = GetResourceDescription().Format,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D{
					.MipSlice = 0,
					.PlaneSlice = 0
				}
			};

			return std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>{ std::move(uavDesc) };
		}
	}
}