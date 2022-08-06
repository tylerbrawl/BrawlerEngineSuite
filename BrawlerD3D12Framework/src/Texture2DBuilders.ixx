module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.TextureBuilderBase;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;

namespace Brawler
{
	namespace D3D12
	{
		template <TextureType Type>
		class Texture2DBuilderIMPL final : private TextureBuilderBase<D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D, Type>
		{
		private:
			using BaseType = TextureBuilderBase<D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D, Type>;

		public:
			constexpr Texture2DBuilderIMPL() = default;

			constexpr Texture2DBuilderIMPL(const Texture2DBuilderIMPL& rhs) = default;
			constexpr Texture2DBuilderIMPL& operator=(const Texture2DBuilderIMPL& rhs) = default;

			constexpr Texture2DBuilderIMPL(Texture2DBuilderIMPL&& rhs) noexcept = default;
			constexpr Texture2DBuilderIMPL& operator=(Texture2DBuilderIMPL&& rhs) noexcept = default;

			constexpr void SetTextureDimensions(const std::size_t width, const std::size_t height);
			constexpr void SetMipLevelCount(const std::uint16_t mipCount);
			constexpr void SetTextureFormat(const DXGI_FORMAT format);

			constexpr void AllowUnorderedAccessViews() requires IsUAVSettingAdjustable<Type>;
			constexpr void DenyUnorderedAccessViews() requires IsUAVSettingAdjustable<Type>;

			constexpr void AllowShaderResourceViews() requires IsSRVSettingAdjustable<Type>;
			constexpr void DenyShaderResourceViews() requires IsSRVSettingAdjustable<Type>;

			constexpr void AllowSimultaneousAccess() requires IsSimultaneousAccessSettingAdjustable<Type>;
			constexpr void DenySimultaneousAccess() requires IsSimultaneousAccessSettingAdjustable<Type>;

			constexpr void SetInitialResourceState(const D3D12_RESOURCE_STATES initialState) requires IsInitialResourceStateAdjustable<Type>;

			template <typename T>
				requires std::is_same_v<std::decay_t<T>, DXGI_SAMPLE_DESC>
			constexpr void SetMultisampleDescription(T&& sampleDesc) requires IsMultisamplingSettingAdjustable<D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D, Type>;

			template <typename T>
				requires std::is_same_v<std::decay_t<T>, D3D12_CLEAR_VALUE>
			constexpr void SetOptimizedClearValue(T&& clearValue) requires NeedsSpecialResourceInitialization<Type>;

			constexpr void SetPreferredSpecialInitializationMethod(const GPUResourceSpecialInitializationMethod initMethod) requires NeedsSpecialResourceInitialization<Type>;

			constexpr const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;
			constexpr D3D12_RESOURCE_STATES GetInitialResourceState() const;
			constexpr const std::optional<D3D12_CLEAR_VALUE>& GetOptimizedClearValue() const;
			constexpr GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetTextureDimensions(const std::size_t width, const std::size_t height)
		{
			BaseType::SetTextureWidth(width);
			BaseType::SetTextureHeight(height);
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetMipLevelCount(const std::uint16_t mipCount)
		{
			BaseType::SetMipLevelCount(mipCount);
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetTextureFormat(const DXGI_FORMAT format)
		{
			BaseType::SetTextureFormat(format);
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::AllowUnorderedAccessViews() requires IsUAVSettingAdjustable<Type>
		{
			BaseType::AllowUnorderedAccessViews();
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::DenyUnorderedAccessViews() requires IsUAVSettingAdjustable<Type>
		{
			BaseType::DenyUnorderedAccessViews();
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::AllowShaderResourceViews() requires IsSRVSettingAdjustable<Type>
		{
			BaseType::AllowShaderResourceViews();
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::DenyShaderResourceViews() requires IsSRVSettingAdjustable<Type>
		{
			BaseType::DenyShaderResourceViews();
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::AllowSimultaneousAccess() requires IsSimultaneousAccessSettingAdjustable<Type>
		{
			BaseType::AllowSimultaneousAccess();
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::DenySimultaneousAccess() requires IsSimultaneousAccessSettingAdjustable<Type>
		{
			BaseType::DenySimultaneousAccess();
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetInitialResourceState(const D3D12_RESOURCE_STATES initialState) requires IsInitialResourceStateAdjustable<Type>
		{
			BaseType::SetInitialResourceState(initialState);
		}

		template <TextureType Type>
		template <typename T>
			requires std::is_same_v<std::decay_t<T>, DXGI_SAMPLE_DESC>
		constexpr void Texture2DBuilderIMPL<Type>::SetMultisampleDescription(T&& sampleDesc) requires IsMultisamplingSettingAdjustable<D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D, Type>
		{
			BaseType::SetMultisampleDescription(std::forward<T>(sampleDesc));
		}

		template <TextureType Type>
		template <typename T>
			requires std::is_same_v<std::decay_t<T>, D3D12_CLEAR_VALUE>
		constexpr void Texture2DBuilderIMPL<Type>::SetOptimizedClearValue(T&& clearValue) requires NeedsSpecialResourceInitialization<Type>
		{
			BaseType::SetOptimizedClearValue(std::forward<T>(clearValue));
		}

		template <TextureType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetPreferredSpecialInitializationMethod(const GPUResourceSpecialInitializationMethod initMethod) requires NeedsSpecialResourceInitialization<Type>
		{
			BaseType::SetPreferredSpecialInitializationMethod(initMethod);
		}

		template <TextureType Type>
		constexpr const Brawler::D3D12_RESOURCE_DESC& Texture2DBuilderIMPL<Type>::GetResourceDescription() const
		{
			return BaseType::GetResourceDescription();
		}

		template <TextureType Type>
		constexpr D3D12_RESOURCE_STATES Texture2DBuilderIMPL<Type>::GetInitialResourceState() const
		{
			return BaseType::GetInitialResourceState();
		}

		template <TextureType Type>
		constexpr const std::optional<D3D12_CLEAR_VALUE>& Texture2DBuilderIMPL<Type>::GetOptimizedClearValue() const
		{
			return BaseType::GetOptimizedClearValue();
		}

		template <TextureType Type>
		constexpr GPUResourceSpecialInitializationMethod Texture2DBuilderIMPL<Type>::GetPreferredSpecialInitializationMethod() const
		{
			return BaseType::GetPreferredSpecialInitializationMethod();
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using Texture2DBuilder = Texture2DBuilderIMPL<TextureType::GENERIC_TEXTURE>;
		using RenderTargetTexture2DBuilder = Texture2DBuilderIMPL<TextureType::RENDER_TARGET>;
		using DepthStencilTextureBuilder = Texture2DBuilderIMPL<TextureType::DEPTH_STENCIL>;
	}
}