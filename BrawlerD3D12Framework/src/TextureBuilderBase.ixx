module;
#include <optional>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.TextureBuilderBase;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_RESOURCE_DIMENSION ResourceDimension>
		concept IsTextureDimension = (ResourceDimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && ResourceDimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_UNKNOWN);
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		enum class TextureType
		{
			GENERIC_TEXTURE,
			RENDER_TARGET,
			DEPTH_STENCIL
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <TextureType Type>
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
			bool EnableSimultaneousAccessOption,
			bool EnableOptimizedClearValueOption
		>
		struct BuilderInfoInstantiation
		{
			/// <summary>
			/// These are the initial flags of the D3D12_RESOURCE_FLAGS instance in the
			/// texture builder.
			/// </summary>
			static constexpr D3D12_RESOURCE_FLAGS DEFAULT_FLAGS = DefaultFlags;

			/// <summary>
			/// This is the initial resource state of the texture. This value may, or may not,
			/// actually be changeable.
			/// </summary>
			static constexpr std::optional<D3D12_RESOURCE_STATES> INITIAL_RESOURCE_STATE = (EnableInitialResourceStateOption ? std::optional<D3D12_RESOURCE_STATES>{} : std::optional<D3D12_RESOURCE_STATES>{ InitialResourceState });

			/// <summary>
			/// If this is true, then UAVs can be made from textures created with this type
			/// of texture builder.
			/// </summary>
			static constexpr bool ENABLE_ALLOW_UAV_OPTION = EnableAllowUAVOption;

			/// <summary>
			/// If this is true, then the option to deny creating SRVs for this texture will
			/// be available for this texture builder.
			/// </summary>
			static constexpr bool ENABLE_DENY_SRV_OPTION = EnableDenySRVOption;

			/// <summary>
			/// If this is true, then the option to change the initial resource state of created
			/// textures will be available for this texture builder.
			/// </summary>
			static constexpr bool ENABLE_INITIAL_RESOURCE_STATE_OPTION = EnableInitialResourceStateOption;

			/// <summary>
			/// If this is true, then the option to change the sampling count and quality for the 
			/// created texture will be available for this texture builder.
			/// </summary>
			static constexpr bool ENABLE_SAMPLE_DESC_OPTION = EnableSampleDescOption;

			/// <summary>
			/// If this is true, then the option to allow the texture to be accessed across queues
			/// simultaneously will be available for this texture builder.
			/// </summary>
			static constexpr bool ENABLE_SIMULTANEOUS_ACCESS_OPTION = EnableSimultaneousAccessOption;

			/// <summary>
			/// If this is true, then the option to change the optimized clear value of created
			/// textures will be available for this texture builder.
			/// </summary>
			static constexpr bool ENABLE_OPTIMIZED_CLEAR_VALUE_OPTION = EnableOptimizedClearValueOption;
		};

		// A lot of the choices here come from AMD's best practices for DirectX 12 resources.
		// Visit https://gpuopen.com/performance/ to reference them. I'm not sure if these tips
		// will help for NVIDIA devices, too, but I'd be surprised if they didn't help at least
		// a little bit.

		template <>
		struct BuilderInfo<TextureType::GENERIC_TEXTURE> : public BuilderInfoInstantiation <
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
			true,

			// Only render targets and depth/stencil textures may have optimized clear
			// values.
			false
		>
		{};

		template <>
		struct BuilderInfo<TextureType::RENDER_TARGET> : public BuilderInfoInstantiation <
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
			false,

			// Only render targets and depth/stencil textures may have optimized clear
			// values.
			true
		>
		{};

		template <>
		struct BuilderInfo<TextureType::DEPTH_STENCIL> : public BuilderInfoInstantiation<
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
			false,

			// Only render targets and depth/stencil textures may have optimized clear
			// values.
			true
		>
		{};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <TextureType Type>
		concept IsUAVSettingAdjustable = BuilderInfo<Type>::ENABLE_ALLOW_UAV_OPTION;

		template <TextureType Type>
		concept IsSRVSettingAdjustable = BuilderInfo<Type>::ENABLE_DENY_SRV_OPTION;

		template <TextureType Type>
		concept IsSimultaneousAccessSettingAdjustable = BuilderInfo<Type>::ENABLE_SIMULTANEOUS_ACCESS_OPTION;

		template <TextureType Type>
		concept IsInitialResourceStateAdjustable = BuilderInfo<Type>::ENABLE_INITIAL_RESOURCE_STATE_OPTION;

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
		concept IsMultisamplingSettingAdjustable = (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D && BuilderInfo<Type>::ENABLE_SAMPLE_DESC_OPTION);

		template <TextureType Type>
		concept NeedsSpecialResourceInitialization = BuilderInfo<Type>::ENABLE_OPTIMIZED_CLEAR_VALUE_OPTION;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		class TextureBuilderBase
		{
		public:
			constexpr TextureBuilderBase();

			constexpr TextureBuilderBase(const TextureBuilderBase& rhs) = default;
			constexpr TextureBuilderBase& operator=(const TextureBuilderBase& rhs) = default;

			constexpr TextureBuilderBase(TextureBuilderBase&& rhs) noexcept = default;
			constexpr TextureBuilderBase& operator=(TextureBuilderBase&& rhs) noexcept = default;

			constexpr void SetTextureWidth(const std::size_t width);
			constexpr void SetTextureHeight(const std::size_t height) requires (ResourceDimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D);

			constexpr void SetTextureDepth(const std::size_t depth) requires (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D);
			constexpr void SetTextureArraySize(const std::size_t arraySize) requires (ResourceDimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D);

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
			constexpr void SetMultisampleDescription(T&& sampleDesc) requires IsMultisamplingSettingAdjustable<ResourceDimension, Type>;

			template <typename T>
				requires std::is_same_v<std::decay_t<T>, D3D12_CLEAR_VALUE>
			constexpr void SetOptimizedClearValue(T&& clearValue) requires NeedsSpecialResourceInitialization<Type>;

			constexpr void SetPreferredSpecialInitializationMethod(const GPUResourceSpecialInitializationMethod initMethod) requires NeedsSpecialResourceInitialization<Type>;

			constexpr const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;
			constexpr D3D12_RESOURCE_STATES GetInitialResourceState() const;
			constexpr const std::optional<D3D12_CLEAR_VALUE>& GetOptimizedClearValue() const;
			constexpr GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const;

		private:
			Brawler::D3D12_RESOURCE_DESC mResourceDesc;
			std::optional<D3D12_RESOURCE_STATES> mInitialResourceState;
			std::optional<D3D12_CLEAR_VALUE> mOptimizedClearValue;
			GPUResourceSpecialInitializationMethod mInitMethod;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr TextureBuilderBase<ResourceDimension, Type>::TextureBuilderBase() :
			mResourceDesc(),
			mInitialResourceState(),
			mOptimizedClearValue(),

			// For performance, always prefer to DISCARD by default.
			mInitMethod(GPUResourceSpecialInitializationMethod::DISCARD)
		{
			mResourceDesc.Dimension = ResourceDimension;
			mResourceDesc.Flags = BuilderInfo<Type>::DEFAULT_FLAGS;

			// All texture dimension values must be in the range [1, X], where X represents the hardware 
			// feature level limit for that particular resource dimension.
			mResourceDesc.Width = 1;
			mResourceDesc.Height = 1;
			mResourceDesc.DepthOrArraySize = 1;

			// Always use 1 as the default mip count. If we leave it as 0, then D3D12 will try to
			// use the maximum number of mip levels.
			mResourceDesc.MipLevels = 1;

			// Likewise, the multisampling description should be set to a default value that disables
			// multisampling, since only render targets and depth/stencil textures support this,
			// anyways.
			mResourceDesc.SampleDesc = DXGI_SAMPLE_DESC{
				.Count = 1,
				.Quality = 0
			};

			// For render targets and depth/stencil textures, set the default optimized clear value
			// to zero. This is consistent with the APIs for clearing views in DirectContext and
			// ComputeContext. (Depth/Stencil textures which are meant to be used as reverse-Z depth
			// buffers will need to explicitly modify the clear value.)
			if constexpr (NeedsSpecialResourceInitialization<Type>)
				mOptimizedClearValue = D3D12_CLEAR_VALUE{};
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetTextureWidth(const std::size_t width)
		{
			assert(width > 0 && "ERROR: A width of 0 was provided when specifying the width of a texture in a call to TextureBuilderBase::SetTextureWidth()!");

			if constexpr (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D)
				assert(width <= D3D12_REQ_TEXTURE1D_U_DIMENSION && "ERROR: The width provided in a call to TextureBuilderBase::SetTextureWidth() was too large for D3D12!");

			else if constexpr (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D)
				assert(width <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION && "ERROR: The width provided in a call to TextureBuilderBase::SetTextureWidth() was too large for D3D12!");

			else
				assert(width <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION && "ERROR: The width provided in a call to TextureBuilderBase::SetTextureWidth() was too large for D3D12!");

			mResourceDesc.Width = width;
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetTextureHeight(const std::size_t height) requires (ResourceDimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D)
		{
			assert(height > 0 && "ERROR: A height of 0 was provided when specifying the width of a texture in a call to TextureBuilderBase::SetTextureHeight()!");
			
			if constexpr (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D)
				assert(height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION && "ERROR: The height provided in a call to TextureBuilderBase::SetTextureHeight() was too large for D3D12!");

			else
				assert(height <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION && "ERROR: The height provided in a call to TextureBuilderBase::SetTextureHeight() was too large for D3D12!");

			mResourceDesc.Height = static_cast<std::uint32_t>(height);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetTextureDepth(const std::size_t depth) requires (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			assert(depth > 0 && "ERROR: A depth of 0 was provided in a call to TextureBuilderBase::SetTextureDepth()!");
			assert(depth <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION && "ERROR: The height provided in a call to TextureBuilderBase<D3D12_RESOURCE_DIMENSION_TEXTURE3D>::SetTextureDepth() was too large for D3D12!");

			mResourceDesc.DepthOrArraySize = static_cast<std::uint16_t>(depth);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetTextureArraySize(const std::size_t arraySize) requires (ResourceDimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D)
		{
			assert(arraySize > 0 && "ERROR: An array size of 0 was provided in a call to TextureBuilderBase::SetTextureArraySize()!");
			
			if constexpr (ResourceDimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D)
				assert(arraySize <= D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION && "ERROR: The array size provided in a call to TextureBuilderBase<D3D12_RESOURCE_DIMENSION_TEXTURE1D>::SetTextureArraySize() was too large for D3D12!");

			else
				assert(arraySize <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION && "ERROR: The array size provided in a call to TextureBuilderBase<D3D12_RESOURCE_DIMENSION_TEXTURE2D>::SetTextureArraySize() was too large for D3D12!");

			mResourceDesc.DepthOrArraySize = static_cast<std::uint16_t>(arraySize);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetMipLevelCount(const std::uint16_t mipCount)
		{
			mResourceDesc.MipLevels = mipCount;
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetTextureFormat(const DXGI_FORMAT format)
		{
			mResourceDesc.Format = format;

			// If the clear value is allowed, then set the format associated with it to format.
			if constexpr (BuilderInfo<Type>::ENABLE_OPTIMIZED_CLEAR_VALUE_OPTION)
			{
				assert(mOptimizedClearValue.has_value());
				mOptimizedClearValue->Format = format;
			}
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::AllowUnorderedAccessViews() requires IsUAVSettingAdjustable<Type>
		{
			// The people at Microsoft were nice enough to add explicit C++ bit-wise operator overloads
			// for D3D12 enumeration types... but they forgot to make some of them (e.g., |=, &=, etc.) constexpr!

			mResourceDesc.Flags = (mResourceDesc.Flags | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::DenyUnorderedAccessViews() requires IsUAVSettingAdjustable<Type>
		{
			mResourceDesc.Flags = (mResourceDesc.Flags & ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::AllowShaderResourceViews() requires IsSRVSettingAdjustable<Type>
		{
			mResourceDesc.Flags = (mResourceDesc.Flags & ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::DenyShaderResourceViews() requires IsSRVSettingAdjustable<Type>
		{
			mResourceDesc.Flags = (mResourceDesc.Flags | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::AllowSimultaneousAccess() requires IsSimultaneousAccessSettingAdjustable<Type>
		{
			mResourceDesc.Flags = (mResourceDesc.Flags | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::DenySimultaneousAccess() requires IsSimultaneousAccessSettingAdjustable<Type>
		{
			mResourceDesc.Flags = (mResourceDesc.Flags & ~(D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS));
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetInitialResourceState(const D3D12_RESOURCE_STATES initialState) requires IsInitialResourceStateAdjustable<Type>
		{
			mInitialResourceState = initialState;
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		template <typename T>
			requires std::is_same_v<std::decay_t<T>, DXGI_SAMPLE_DESC>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetMultisampleDescription(T&& sampleDesc) requires IsMultisamplingSettingAdjustable<ResourceDimension, Type>
		{
			mResourceDesc.SampleDesc = std::forward<T>(sampleDesc);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		template <typename T>
			requires std::is_same_v<std::decay_t<T>, D3D12_CLEAR_VALUE>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetOptimizedClearValue(T&& clearValue) requires NeedsSpecialResourceInitialization<Type>
		{
			mOptimizedClearValue = std::forward<T>(clearValue);

			// Set the format based on what is currently present in the resource description.
			mOptimizedClearValue->Format = mResourceDesc.Format;
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr void TextureBuilderBase<ResourceDimension, Type>::SetPreferredSpecialInitializationMethod(const GPUResourceSpecialInitializationMethod initMethod) requires NeedsSpecialResourceInitialization<Type>
		{
			mInitMethod = initMethod;
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr const Brawler::D3D12_RESOURCE_DESC& TextureBuilderBase<ResourceDimension, Type>::GetResourceDescription() const
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

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr D3D12_RESOURCE_STATES TextureBuilderBase<ResourceDimension, Type>::GetInitialResourceState() const
		{
			// Simultaneous-access textures are implicitly promoted from the COMMON state on their 
			// first use. So, it makes sense to start them in the COMMON state.
			const bool isSimultaneousAccess = ((mResourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);
			assert((mInitialResourceState.has_value() || isSimultaneousAccess) && "ERROR: A texture builder for a texture which is not a render target, depth/stencil texture, or a simultaneous-access texture was never assigned an initial resource state!");

			return (isSimultaneousAccess ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON : *mInitialResourceState);
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr const std::optional<D3D12_CLEAR_VALUE>& TextureBuilderBase<ResourceDimension, Type>::GetOptimizedClearValue() const
		{
			return mOptimizedClearValue;
		}

		template <D3D12_RESOURCE_DIMENSION ResourceDimension, TextureType Type>
			requires IsTextureDimension<ResourceDimension>
		constexpr GPUResourceSpecialInitializationMethod TextureBuilderBase<ResourceDimension, Type>::GetPreferredSpecialInitializationMethod() const
		{
			return mInitMethod;
		}
	}
}