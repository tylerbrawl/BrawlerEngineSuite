module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.DepthStencilTexture;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;

export namespace Brawler
{
	namespace D3D12
	{
		class DepthStencilTexture final : public I_GPUResource
		{
		public:
			explicit DepthStencilTexture(const DepthStencilTextureBuilder& builder);

			DepthStencilTexture(const DepthStencilTexture& rhs) = delete;
			DepthStencilTexture& operator=(const DepthStencilTexture& rhs) = delete;

			DepthStencilTexture(DepthStencilTexture&& rhs) noexcept = default;
			DepthStencilTexture& operator=(DepthStencilTexture&& rhs) noexcept = default;

			std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const override;
			GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const override;

		private:
			std::optional<D3D12_CLEAR_VALUE> mOptimizedClearValue;
			GPUResourceSpecialInitializationMethod mInitMethod;
		};
	}
}