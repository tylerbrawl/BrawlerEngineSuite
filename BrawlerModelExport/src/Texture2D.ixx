module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.Texture2D;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		struct Texture2DInitializationInfo
		{
			std::uint64_t Width;
			std::uint32_t Height;
			std::uint16_t MipLevels;
			DXGI_FORMAT Format;
			D3D12_TEXTURE_LAYOUT Layout;
			D3D12_RESOURCE_FLAGS Flags;
			D3D12MA::ALLOCATION_DESC AllocationDesc;
			D3D12_RESOURCE_STATES InitialResourceState;
		};

		class Texture2D final : public I_GPUResource
		{
		public:
			explicit Texture2D(const Texture2DInitializationInfo& initInfo);

			Texture2D(const Texture2D& rhs) = delete;
			Texture2D& operator=(const Texture2D& rhs) = delete;

			Texture2D(Texture2D&& rhs) noexcept = default;
			Texture2D& operator=(Texture2D&& rhs) noexcept = default;

			std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> CreateSRVDescription() const override;
			std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> CreateUAVDescription() const override;
		};
	}
}