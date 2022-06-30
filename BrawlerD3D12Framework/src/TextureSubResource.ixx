module;
#include "DxDef.h"

export module Brawler.D3D12.TextureSubResource;
import Brawler.D3D12.I_GPUResource;

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
		protected:
			explicit TextureSubResource(const std::uint32_t subResourceIndex);

		public:
			virtual ~TextureSubResource() = default;

			TextureSubResource(const TextureSubResource& rhs) = default;
			TextureSubResource& operator=(const TextureSubResource& rhs) = default;

			TextureSubResource(TextureSubResource&& rhs) noexcept = default;
			TextureSubResource& operator=(TextureSubResource&& rhs) noexcept = default;

			virtual I_GPUResource& GetGPUResource() = 0;
			virtual const I_GPUResource& GetGPUResource() const = 0;

			Brawler::D3D12Resource& GetD3D12Resource() const;
			std::uint32_t GetSubResourceIndex() const;

			const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;

		private:
			std::uint32_t mSubResourceIndex;
		};
	}
}