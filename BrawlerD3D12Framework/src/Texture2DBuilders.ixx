module;
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.Texture2DBuilders;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		enum class BuilderType
		{
			GENERIC_TEXTURE_2D,
			RENDER_TARGET,
			DEPTH_STENCIL
		};

		template <BuilderType Type>
		struct BuilderInfo
		{
			static_assert(sizeof(Type) != sizeof(Type));
		};

		template <
			D3D12_RESOURCE_FLAGS DefaultFlags,

			// std::optional doesn't appear to be a valid non-type template parameter, so we'll
			// have to work around it.
			D3D12_RESOURCE_STATES InitialResourceState,

			bool EnableAllowUAVOption,
			bool EnableDenySRVOption,
			bool EnableInitialResourceStateOption,
			bool EnableSampleDescOption,
			bool EnableSimultaneousAccessOption
		>
		struct BuilderInfoInstantiation
		{
			/// <summary>
			/// These are the initial flags of the D3D12_RESOURCE_FLAGS instance in the
			/// Texture2DBuilderIMPL.
			/// </summary>
			static constexpr D3D12_RESOURCE_FLAGS DEFAULT_FLAGS = DefaultFlags;

			/// <summary>
			/// This is the initial resource state of the texture. This value may, or may not,
			/// actually be changeable.
			/// </summary>
			static constexpr std::optional<D3D12_RESOURCE_STATES> INITIAL_RESOURCE_STATE = (EnableInitialResourceStateOption ? std::optional<D3D12_RESOURCE_STATES>{} : std::optional<D3D12_RESOURCE_STATES>{ InitialResourceState });

			/// <summary>
			/// If this is true, then UAVs can be made from textures created with this type
			/// of Texture2DBuilderIMPL.
			/// </summary>
			static constexpr bool ENABLE_ALLOW_UAV_OPTION = EnableAllowUAVOption;

			/// <summary>
			/// If this is true, then the option to deny creating SRVs for this texture will
			/// be available for this Texture2DBuilderIMPL.
			/// </summary>
			static constexpr bool ENABLE_DENY_SRV_OPTION = EnableDenySRVOption;

			/// <summary>
			/// If this is true, then the option to change the initial resource state of created
			/// textures will be available for this Texture2DBuilderIMPL.
			/// </summary>
			static constexpr bool ENABLE_INITIAL_RESOURCE_STATE_OPTION = EnableInitialResourceStateOption;

			/// <summary>
			/// If this is true, then the option to change the sampling count and quality for the 
			/// created texture will be available for this Texture2DBuilderIMPL.
			/// </summary>
			static constexpr bool ENABLE_SAMPLE_DESC_OPTION = EnableSampleDescOption;

			/// <summary>
			/// If this is true, then the option to allow the texture to be accessed across queues
			/// simultaneously will be available for this Texture2DBuilderIMPL.
			/// </summary>
			static constexpr bool ENABLE_SIMULTANEOUS_ACCESS_OPTION = EnableSimultaneousAccessOption;
		};

		// A lot of the choices here come from AMD's best practices for DirectX 12 resources.
		// Visit https://gpuopen.com/performance/ to reference them. I'm not sure if these tips
		// will help for NVIDIA devices, too, but I'd be surprised if they didn't help at least
		// a little bit.

		template <>
		struct BuilderInfo<BuilderType::GENERIC_TEXTURE_2D> : public BuilderInfoInstantiation<
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,

			// We should only ever make UAVs out of a texture if they actually need to be
			// written to, as marking a resource as read-only can allow for compression
			// optimizations. Thus, rather than always enabling UAVs, we make it an option.
			true,

			// The DENY_SHADER_RESOURCE flag is only meant for depth/stencil textures.
			false,

			// We don't actually want textures to start in the D3D12_RESOURCE_STATE_COMMON
			// state. We just can't tell right now what state a texture will need to start
			// in.
			true,

			// Multisampling is only available for render targets and depth/stencil textures.
			false,

			// If necessary, general textures can be made simultaneously accessible across
			// multiple queues.
			true
		>
		{};

		template <>
		struct BuilderInfo<BuilderType::RENDER_TARGET> : public BuilderInfoInstantiation<
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,

			// The first render operation on a render target must be a special
			// resource initialization operation, so we need the resource to start in
			// the RENDER_TARGET state.
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,

			// According to AMD, UAVs shouldn't be made out of render targets, since it
			// can prevent color compression on older hardware.
			false,

			// The DENY_SHADER_RESOURCE flag is only meant for depth/stencil textures.
			false,

			// The Brawler Engine requires that render targets start in a specific
			// state.
			false,

			// Multisampling is only available for render targets and depth/stencil textures.
			true,

			// AMD suggests avoiding simultaneous access for render targets and depth/stencil
			// textures.
			false
		>
		{};

		template <>
		struct BuilderInfo<BuilderType::DEPTH_STENCIL> : public BuilderInfoInstantiation<
			// By default, disallow SRVs for depth/stencil textures, since this can
			// allow for compression.
			D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,

			// The first render operation on a depth/stencil texture must be a special
			// resource initialization operation, so we need the resource to start in
			// the DEPTH_WRITE state.
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE,

			// Even if we wanted to make a UAV out of a depth/stencil texture for some
			// unholy reason, the D3D12 API disallows it.
			false,

			// The DENY_SHADER_RESOURCE flag is only meant for depth/stencil textures.
			true,

			// The Brawler Engine requires that depth/stencil textures start in a specific
			// state.
			false,

			// Multisampling is only available for render targets and depth/stencil textures.
			true,

			// AMD suggests avoiding simultaneous access for render targets and depth/stencil
			// textures.
			false
		>
		{};

		template <BuilderType Type>
		class Texture2DBuilderIMPL
		{
		public:
			constexpr Texture2DBuilderIMPL();

			constexpr Texture2DBuilderIMPL(const Texture2DBuilderIMPL& rhs) = default;
			constexpr Texture2DBuilderIMPL& operator=(const Texture2DBuilderIMPL& rhs) = default;

			constexpr Texture2DBuilderIMPL(Texture2DBuilderIMPL&& rhs) noexcept = default;
			constexpr Texture2DBuilderIMPL& operator=(Texture2DBuilderIMPL&& rhs) noexcept = default;

			constexpr void SetTextureDimensions(const std::size_t width, const std::size_t height);
			constexpr void SetMipLevelCount(const std::uint16_t mipCount);
			constexpr void SetTextureFormat(const DXGI_FORMAT format);

			constexpr void AllowUnorderedAccessViews() requires BuilderInfo<Type>::ENABLE_ALLOW_UAV_OPTION;
			constexpr void DenyUnorderedAccessViews() requires BuilderInfo<Type>::ENABLE_ALLOW_UAV_OPTION;

			constexpr void AllowShaderResourceViews() requires BuilderInfo<Type>::ENABLE_DENY_SRV_OPTION;
			constexpr void DenyShaderResourceViews() requires BuilderInfo<Type>::ENABLE_DENY_SRV_OPTION;

			constexpr void AllowSimultaneousAccess() requires BuilderInfo<Type>::ENABLE_SIMULTANEOUS_ACCESS_OPTION;
			constexpr void DenySimultaneousAccess() requires BuilderInfo<Type>::ENABLE_SIMULTANEOUS_ACCESS_OPTION;

			constexpr void SetInitialResourceState(const D3D12_RESOURCE_STATES initialState) requires BuilderInfo<Type>::ENABLE_INITIAL_RESOURCE_STATE_OPTION;
			constexpr D3D12_RESOURCE_STATES GetInitialResourceState() const;

			constexpr void SetMultisampleDescription(DXGI_SAMPLE_DESC&& sampleDesc) requires BuilderInfo<Type>::ENABLE_SAMPLE_DESC_OPTION;

			constexpr const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;

		private:
			Brawler::D3D12_RESOURCE_DESC mResourceDesc;
			std::optional<D3D12_RESOURCE_STATES> mInitialResourceState;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <BuilderType Type>
		constexpr Texture2DBuilderIMPL<Type>::Texture2DBuilderIMPL() :
			mResourceDesc(),
			mInitialResourceState(BuilderInfo<Type>::INITIAL_RESOURCE_STATE)
		{
			mResourceDesc.DepthOrArraySize = 1;
			
			// By default, the mip level count should be 1. This will allow supporting resources
			// which cannot have more than one mip level by default, and should likely be the common
			// case.
			mResourceDesc.MipLevels = 1;

			// Likewise, the multisampling description should be set to a default value that disables
			// multisampling, since only render targets and depth/stencil textures support this,
			// anyways.
			mResourceDesc.SampleDesc = DXGI_SAMPLE_DESC{
				.Count = 1,
				.Quality = 0
			};

			mResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			mResourceDesc.Flags = BuilderInfo<Type>::DEFAULT_FLAGS;
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetTextureDimensions(const std::size_t width, const std::size_t height)
		{
			assert((width <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) && "ERROR: The width provided in a call to Texture2DBuilderIMPL::SetTextureDimensions() was too large for D3D12!");
			assert((height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) && "ERROR: The height provided in a call to Texture2DBuilderIMPL::SetTextureDimensions() was too large for D3D12!");

			mResourceDesc.Width = width;
			mResourceDesc.Height = static_cast<std::uint32_t>(height);
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetMipLevelCount(const std::uint16_t mipCount)
		{
			mResourceDesc.MipLevels = mipCount;
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetTextureFormat(const DXGI_FORMAT format)
		{
			mResourceDesc.Format = format;
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::AllowUnorderedAccessViews() requires BuilderInfo<Type>::ENABLE_ALLOW_UAV_OPTION
		{
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::DenyUnorderedAccessViews() requires BuilderInfo<Type>::ENABLE_ALLOW_UAV_OPTION
		{
			mResourceDesc.Flags &= ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::AllowShaderResourceViews() requires BuilderInfo<Type>::ENABLE_DENY_SRV_OPTION
		{
			mResourceDesc.Flags &= ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::DenyShaderResourceViews() requires BuilderInfo<Type>::ENABLE_DENY_SRV_OPTION
		{
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::AllowSimultaneousAccess() requires BuilderInfo<Type>::ENABLE_SIMULTANEOUS_ACCESS_OPTION
		{
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::DenySimultaneousAccess() requires BuilderInfo<Type>::ENABLE_SIMULTANEOUS_ACCESS_OPTION
		{
			mResourceDesc.Flags &= ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetInitialResourceState(const D3D12_RESOURCE_STATES initialState) requires BuilderInfo<Type>::ENABLE_INITIAL_RESOURCE_STATE_OPTION
		{
			mInitialResourceState = initialState;
		}

		template <BuilderType Type>
		constexpr D3D12_RESOURCE_STATES Texture2DBuilderIMPL<Type>::GetInitialResourceState() const
		{
			// Simultaneous-access textures are implicitly promoted from the COMMON state on their 
			// first use. So, it makes sense to start them in the COMMON state.
			const bool isSimultaneousAccess = ((mResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);
			assert((mInitialResourceState.has_value() || isSimultaneousAccess) && "ERROR: A Texture2DBuilder for a Texture2D which is not a render target, a depth/stencil texture, or a simultaneous-access texture was never assigned an initial resource state!");

			return (isSimultaneousAccess ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON : *mInitialResourceState);
		}

		template <BuilderType Type>
		constexpr void Texture2DBuilderIMPL<Type>::SetMultisampleDescription(DXGI_SAMPLE_DESC&& sampleDesc) requires BuilderInfo<Type>::ENABLE_SAMPLE_DESC_OPTION
		{
			mResourceDesc.SampleDesc = std::move(sampleDesc);
		}

		template <BuilderType Type>
		constexpr const Brawler::D3D12_RESOURCE_DESC& Texture2DBuilderIMPL<Type>::GetResourceDescription() const
		{
			// Perform additional sanity checks in Debug builds.

			if constexpr (Util::General::IsDebugModeEnabled())
			{
				if (mResourceDesc.SampleDesc.Count > 1 || mResourceDesc.SampleDesc.Quality != 0)
				{
					assert((mResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0 && "ERROR: Multisampling is not available for textures which are to be used with UAVs! To use multisampling, disallow unordered access views.");
					assert(mResourceDesc.MipLevels == 1 && "ERROR: Multisampling disallows any mip-level count other than 1!");
				}
			}

			return mResourceDesc;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using Texture2DBuilder = Texture2DBuilderIMPL<BuilderType::GENERIC_TEXTURE_2D>;
		using RenderTargetBuilder = Texture2DBuilderIMPL<BuilderType::RENDER_TARGET>;
		using DepthStencilTextureBuilder = Texture2DBuilderIMPL<BuilderType::DEPTH_STENCIL>;
	}
}