module;
#include "DxDef.h"

export module Brawler.D3D12.TextureSubResource;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
		class Texture2D;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		/// <summary>
		/// The D3D12 API has a somewhat arbitrary definition for sub-resource indices. The details
		/// for calculating a sub-resource index for a given resource type can be found on the MSDN at
		/// https://docs.microsoft.com/en-us/windows/win32/direct3d12/subresources.
		/// 
		/// The idea is that derived classes of I_GPUResource which refer to texture resource types
		/// can offer a convenient API for creating a TextureSubResource instance based on a set
		/// of relevant parameters and the use of the D3D12CalcSubresource() function from d3dx12.h.
		/// For instance, depth/stencil texture resource types could offer two separate functions
		/// for getting a TextureSubResource for either the depth plane or the stencil plane.
		/// 
		/// Ideally, calling code should never have to call D3D12CalcSubresource() themselves to
		/// create TextureSubResource instances.
		/// </summary>
		class TextureSubResource
		{
			// To ensure that only texture resource types can be used to create TextureSubResource
			// instances, we can add a constructor for each base texture type.

		public:
			TextureSubResource() = default;
			TextureSubResource(const Texture2D& texture2D, const std::uint32_t subResourceIndex);

			TextureSubResource(const TextureSubResource& rhs) = default;
			TextureSubResource& operator=(const TextureSubResource& rhs) = default;

			TextureSubResource(TextureSubResource&& rhs) noexcept = default;
			TextureSubResource& operator=(TextureSubResource&& rhs) noexcept = default;

			const I_GPUResource& GetGPUResource() const;

			Brawler::D3D12Resource& GetD3D12Resource() const;
			std::uint32_t GetSubResourceIndex() const;

			const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;

		private:
			const I_GPUResource* mResourcePtr;
			std::uint32_t mSubResourceIndex;
		};
	}
}