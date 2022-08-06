module;
#include <optional>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.Texture2DArray;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.Texture2DArrayBuilders;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;
import Brawler.D3D12.GPUResourceViews;
import Util.Engine;

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2DArray;
		class Texture2DArraySlice;
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2DArraySubResource final : public TextureSubResource
		{
		private:
			struct SubResourceInfo
			{
				std::uint32_t ArraySlice;
				std::uint32_t MipLevel;
				std::uint32_t PlaneSlice;
			};

		public:
			Texture2DArraySubResource() = default;
			Texture2DArraySubResource(Texture2DArray& tex2DArr, const std::uint32_t subResourceIndex);

			Texture2DArraySubResource(const Texture2DArraySubResource& rhs) = default;
			Texture2DArraySubResource& operator=(const Texture2DArraySubResource& rhs) = default;

			Texture2DArraySubResource(Texture2DArraySubResource&& rhs) noexcept = default;
			Texture2DArraySubResource& operator=(Texture2DArraySubResource&& rhs) noexcept = default;

			I_GPUResource& GetGPUResource() override;
			const I_GPUResource& GetGPUResource() const override;

			Texture2DArray& GetTexture2DArray();
			const Texture2DArray& GetTexture2DArray() const;

			template <DXGI_FORMAT Format>
			Texture2DArrayShaderResourceView<Format> CreateShaderResourceView() const;

			template <DXGI_FORMAT Format>
			Texture2DArrayUnorderedAccessView<Format> CreateUnorderedAccessView() const;

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV();

		private:
			SubResourceInfo GetSubResourceInfo() const;

		private:
			Texture2DArray* mTexArrayPtr;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DArrayShaderResourceView<Format> Texture2DArraySubResource::CreateShaderResourceView() const
		{
			// Considering that a single sub-resource in a Texture2DArray is just a mip level of a single texture,
			// one might think that creating a Texture2DShaderResourceView would be a better option. I would
			// agree with that sentiment, if it weren't for the fact that the MSDN states the following about
			// the ViewDimension member of the D3D12_SHADER_RESOURCE_VIEW_DESC struct:
			//
			// This type is the same as the resource type of the underlying resource. This member also determines
			// which _SRV to use in the union...
			//
			// So, even if we are viewing just a single sub-resource, we still need to use a 
			// Texture2DArrayShaderResourceView, since the underlying resource is a Texture2DArray. (The source for 
			// this information is
			// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_shader_resource_view_desc.)

			const SubResourceInfo subResourceInfo{ GetSubResourceInfo() };
			return Texture2DArrayShaderResourceView<Format>{ GetTexture2DArray(), D3D12_TEX2D_ARRAY_SRV{
				.MostDetailedMip = subResourceInfo.MipLevel,
				.MipLevels = 1,
				.FirstArraySlice = subResourceInfo.ArraySlice,
				.ArraySize = 1,
				.PlaneSlice = subResourceInfo.PlaneSlice,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DArrayUnorderedAccessView<Format> Texture2DArraySubResource::CreateUnorderedAccessView() const
		{
			// Interestingly, for D3D12_UNORDERED_ACCESS_VIEW_DESC, the description for the ViewDimension member
			// in the MSDN does not explicitly state that this value needs to match the type of the underlying 
			// resource. This probably an oversight, though, so we'll stick with returning a
			// Texture2DArrayUnorderedAccessView instance.

			const SubResourceInfo subResourceInfo{ GetSubResourceInfo() };
			return Texture2DArrayUnorderedAccessView<Format>{ GetTexture2DArray(), D3D12_TEX2D_ARRAY_UAV{
				.MipSlice = subResourceInfo.MipLevel,
				.FirstArraySlice = subResourceInfo.ArraySlice,
				.ArraySize = 1,
				.PlaneSlice = subResourceInfo.PlaneSlice
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation Texture2DArraySubResource::CreateBindlessSRV()
		{
			const Texture2DArrayShaderResourceView<Format> subResourceSRV{ CreateShaderResourceView<Format>() };
			I_GPUResource& texture2DArrayResource{ static_cast<I_GPUResource&>(GetTexture2DArray()) };

			return texture2DArrayResource.CreateBindlessSRV(subResourceSRV.CreateSRVDescription());
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2DArraySlice
		{
		public:
			Texture2DArraySlice() = default;
			Texture2DArraySlice(Texture2DArray& tex2DArr, const std::uint32_t arraySliceIndex);

			Texture2DArraySlice(const Texture2DArraySlice& rhs) = default;
			Texture2DArraySlice& operator=(const Texture2DArraySlice& rhs) = default;

			Texture2DArraySlice(Texture2DArraySlice&& rhs) noexcept = default;
			Texture2DArraySlice& operator=(Texture2DArraySlice&& rhs) noexcept = default;

			Texture2DArray& GetTexture2DArray();
			const Texture2DArray& GetTexture2DArray() const;

			Texture2DArraySubResource GetSubResource(const std::uint32_t mipSlice);

			template <DXGI_FORMAT Format>
			Texture2DArrayShaderResourceView<Format> CreateShaderResourceView(const std::uint32_t mostDetailedMipOffset = 0) const;

			template <DXGI_FORMAT Format>
			Texture2DArrayUnorderedAccessView<Format> CreateUnorderedAccessView(const std::uint32_t mipSlice = 0) const;

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV(const std::uint32_t mostDetailedMipOffset = 0);

		private:
			Texture2DArray* mTexArrayPtr;
			std::uint32_t mArraySliceIndex;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DArrayShaderResourceView<Format> Texture2DArraySlice::CreateShaderResourceView(const std::uint32_t mostDetailedMipOffset) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			return Texture2DArrayShaderResourceView<Format>{ GetTexture2DArray(), D3D12_TEX2D_ARRAY_SRV{
				.MostDetailedMip = mostDetailedMipOffset,
				.MipLevels = static_cast<std::uint32_t>(-1),
				.FirstArraySlice = mArraySliceIndex,
				.ArraySize = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DArrayUnorderedAccessView<Format> Texture2DArraySlice::CreateUnorderedAccessView(const std::uint32_t mipSlice) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			return Texture2DArrayUnorderedAccessView<Format>{ GetTexture2DArray(), D3D12_TEX2D_ARRAY_UAV{
				.MipSlice = mipSlice,
				.FirstArraySlice = mArraySliceIndex,
				.ArraySize = 1,
				.PlaneSlice = 0
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation Texture2DArraySlice::CreateBindlessSRV(const std::uint32_t mostDetailedMipOffset)
		{
			const Texture2DArrayShaderResourceView<Format> arraySliceSRV{ CreateShaderResourceView(mostDetailedMipOffset) };
			I_GPUResource& tex2DArrayResource{ static_cast<I_GPUResource&>(GetTexture2DArray()) };

			return tex2DArrayResource.CreateBindlessSRV(arraySliceSRV.CreateSRVDescription());
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2DArraySliceRange
		{
		public:
			Texture2DArraySliceRange() = default;
			Texture2DArraySliceRange(Texture2DArray& tex2DArr, const std::uint32_t firstSliceIndex, const std::uint32_t numSlices);

			Texture2DArraySliceRange(const Texture2DArraySliceRange& rhs) = default;
			Texture2DArraySliceRange& operator=(const Texture2DArraySliceRange& rhs) = default;

			Texture2DArraySliceRange(Texture2DArraySliceRange&& rhs) noexcept = default;
			Texture2DArraySliceRange& operator=(Texture2DArraySliceRange&& rhs) noexcept = default;

			Texture2DArray& GetTexture2DArray();
			const Texture2DArray& GetTexture2DArray() const;

			Texture2DArraySlice GetArraySlice(const std::uint32_t relativeSliceIndex);

			template <DXGI_FORMAT Format>
			Texture2DArrayShaderResourceView<Format> CreateShaderResourceView(const std::uint32_t mostDetailedMipOffset = 0) const;

			template <DXGI_FORMAT Format>
			Texture2DArrayUnorderedAccessView<Format> CreateUnorderedAccessView(const std::uint32_t mipSlice = 0) const;

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV(const std::uint32_t mostDetailedMipOffset);

		private:
			Texture2DArray* mTexArrayPtr;
			std::uint32_t mFirstSliceIndex;
			std::uint32_t mNumSlices;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DArrayShaderResourceView<Format> Texture2DArraySliceRange::CreateShaderResourceView(const std::uint32_t mostDetailedMipOffset) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");
			assert(mNumSlices > 0);
			
			return Texture2DArrayShaderResourceView<Format>{ GetTexture2DArray(), D3D12_TEX2D_ARRAY_SRV{
				.MostDetailedMip = mostDetailedMipOffset,
				.MipLevels = static_cast<std::uint32_t>(-1),
				.FirstArraySlice = mFirstSliceIndex,
				.ArraySize = mNumSlices,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DArrayUnorderedAccessView<Format> Texture2DArraySliceRange::CreateUnorderedAccessView(const std::uint32_t mipSlice) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");
			assert(mNumSlices > 0);

			return Texture2DArrayUnorderedAccessView<Format>{ GetTexture2DArray(), D3D12_TEX2D_ARRAY_UAV{
				.MipSlice = mipSlice,
				.FirstArraySlice = mFirstSliceIndex,
				.ArraySize = mNumSlices,
				.PlaneSlice = 0
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation Texture2DArraySliceRange::CreateBindlessSRV(const std::uint32_t mostDetailedMipOffset)
		{
			const Texture2DArrayShaderResourceView<Format> arraySliceRangeSRV{ CreateShaderResourceView(mostDetailedMipOffset) };
			I_GPUResource& tex2DArrayResource{ static_cast<I_GPUResource&>(GetTexture2DArray()) };

			return tex2DArrayResource.CreateBindlessSRV(arraySliceRangeSRV.CreateSRVDescription());
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		class Texture2DArray final : public I_GPUResource
		{
		public:
			explicit Texture2DArray(const Texture2DArrayBuilder& builder);
			explicit Texture2DArray(const RenderTargetTexture2DArrayBuilder& builder);

			Texture2DArray(const Texture2DArray& rhs) = delete;
			Texture2DArray& operator=(const Texture2DArray& rhs) = delete;

			Texture2DArray(Texture2DArray&& rhs) noexcept = default;
			Texture2DArray& operator=(Texture2DArray&& rhs) noexcept = default;

			std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const override;
			GPUResourceSpecialInitializationMethod GetPreferredSpecialInitializationMethod() const override;

			Texture2DArraySlice GetArraySlice(const std::uint32_t arraySliceIndex);
			Texture2DArraySliceRange GetArraySliceRange(const std::uint32_t firstSliceIndex, const std::uint32_t numSlices);

			template <DXGI_FORMAT Format>
			Texture2DArrayShaderResourceView<Format> CreateShaderResourceView(const std::uint32_t mostDetailedMipOffset = 0) const;

			template <DXGI_FORMAT Format>
			Texture2DArrayUnorderedAccessView<Format> CreateUnorderedAccessView(const std::uint32_t mipSlice = 0) const;

			template <DXGI_FORMAT Format>
			BindlessSRVAllocation CreateBindlessSRV(const std::uint32_t mostDetailedMipOffset = 0);

		private:
			std::optional<D3D12_CLEAR_VALUE> mOptimizedClearValue;
			GPUResourceSpecialInitializationMethod mInitMethod;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
		Texture2DArrayShaderResourceView<Format> Texture2DArray::CreateShaderResourceView(const std::uint32_t mostDetailedMipOffset) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetResourceDescription() };
			return Texture2DArrayShaderResourceView<Format>{ *this, D3D12_TEX2D_ARRAY_SRV{
				.MostDetailedMip = mostDetailedMipOffset,
				.MipLevels = static_cast<std::uint32_t>(-1),
				.FirstArraySlice = 0,
				.ArraySize = resourceDesc.DepthOrArraySize,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			} };
		}

		template <DXGI_FORMAT Format>
		Texture2DArrayUnorderedAccessView<Format> Texture2DArray::CreateUnorderedAccessView(const std::uint32_t mipSlice) const
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), Format) == 1 && "ERROR: Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetResourceDescription() };
			return Texture2DArrayUnorderedAccessView<Format>{ *this, D3D12_TEX2D_ARRAY_UAV{
				.MipSlice = mipSlice,
				.FirstArraySlice = 0,
				.ArraySize = resourceDesc.DepthOrArraySize,
				.PlaneSlice = 0
			} };
		}

		template <DXGI_FORMAT Format>
		BindlessSRVAllocation Texture2DArray::CreateBindlessSRV(const std::uint32_t mostDetailedMipOffset)
		{
			return I_GPUResource::CreateBindlessSRV(CreateShaderResourceView<Format>(mostDetailedMipOffset).CreateSRVDescription());
		}
	}
}