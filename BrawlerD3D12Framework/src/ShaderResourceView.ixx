module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.ShaderResourceView;
import :SRVDimensionInfo;
import Util.D3D12;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		class ShaderResourceView;

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_SRV_DIMENSION ViewDimension>
			requires (Util::D3D12::IsSRVRTVDSVResourceCastLegal(FromFormat, ToFormat))
		ShaderResourceView<ToFormat, ViewDimension> ReinterpretResourceCast(const ShaderResourceView<FromFormat, ViewDimension>& srcSRV);

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		consteval D3D12_SHADER_RESOURCE_VIEW_DESC CreateDefaultSRVDescription()
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = Format;
			srvDesc.ViewDimension = ViewDimension;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			return srvDesc;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		class ShaderResourceView
		{
		private:
			template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat>
				requires (Util::D3D12::IsSRVRTVDSVResourceCastLegal(FromFormat, ToFormat))
			friend ShaderResourceView<ToFormat, ViewDimension> ReinterpretResourceCast(const ShaderResourceView<FromFormat, ViewDimension>& srcSrv);

		private:
			using ViewDescType = typename SRVDimensionInfo<ViewDimension>::SRVDescType;

		public:
			ShaderResourceView() = default;
			ShaderResourceView(const I_GPUResource& resource, ViewDescType&& viewDesc);

			ShaderResourceView(const ShaderResourceView& rhs) = default;
			ShaderResourceView& operator=(const ShaderResourceView& rhs) = default;

			ShaderResourceView(ShaderResourceView&& rhs) noexcept = default;
			ShaderResourceView& operator=(ShaderResourceView&& rhs) noexcept = default;

			const I_GPUResource& GetGPUResource() const;
			Brawler::D3D12Resource& GetD3D12Resource() const;

			D3D12_SHADER_RESOURCE_VIEW_DESC CreateSRVDescription() const;

		private:
			const I_GPUResource* mResourcePtr;
			ViewDescType mViewDesc;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		ShaderResourceView<Format, ViewDimension>::ShaderResourceView(const I_GPUResource& resource, ViewDescType&& viewDesc) :
			mResourcePtr(&resource),
			mViewDesc(std::move(viewDesc))
		{
			assert(Util::D3D12::IsSRVRTVDSVResourceCastLegal(resource.GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create a ShaderResourceView with a different format than that of the I_GPUResource which it was supposed to represent, but the cast from the resource's format to that of the SRV was illegal!");
		}

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		const I_GPUResource& ShaderResourceView<Format, ViewDimension>::GetGPUResource() const
		{
			assert(mResourcePtr != nullptr);
			return *mResourcePtr;
		}

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		Brawler::D3D12Resource& ShaderResourceView<Format, ViewDimension>::GetD3D12Resource() const
		{
			assert(mResourcePtr != nullptr);
			return (mResourcePtr->GetD3D12Resource());
		}

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView<Format, ViewDimension>::CreateSRVDescription() const
		{
			// We have to initialize srvDesc in this manner. Doing it any other way causes the MSVC
			// to erroneously crash/break. I don't know why this is the case, but nothing else seems
			// to work.

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = Format;
			srvDesc.ViewDimension = ViewDimension;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			InitializeSRVDescription<ViewDimension>(srvDesc, mViewDesc);

			return srvDesc;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
			requires (Format != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
		using TypedBufferShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER>;

		using StructuredBufferShaderResourceView = ShaderResourceView<DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER>;
		using RawBufferShaderResourceView = ShaderResourceView<DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER>;

		template <DXGI_FORMAT Format>
		using Texture1DShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1D>;

		template <DXGI_FORMAT Format>
		using Texture1DArrayShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE1DARRAY>;

		template <DXGI_FORMAT Format>
		using Texture2DShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D>;

		template <DXGI_FORMAT Format>
		using Texture2DArrayShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DARRAY>;

		// TODO: Do we really want to expose the multisampled 2D texture SRV types?

		template <DXGI_FORMAT Format>
		using Texture3DShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE3D>;

		template <DXGI_FORMAT Format>
		using TextureCubeShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURECUBE>;

		template <DXGI_FORMAT Format>
		using RTASShaderResourceView = ShaderResourceView<Format, D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE>;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		// An internal compiler error (ICE) in the MSVC is preventing us from doing a static_assert on the value
		// of Util::D3D12::IsSRVRTVDSVResourceCastLegal(). To get the original behavior, we now define the function
		// twice, but with different constraints. If the resource cast is legal, then the original function is
		// used; otherwise, a dummy function is used which does a static_assert that always fires when triggered.
		
		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_SRV_DIMENSION ViewDimension>
			requires (Util::D3D12::IsSRVRTVDSVResourceCastLegal(FromFormat, ToFormat))
		ShaderResourceView<ToFormat, ViewDimension> ReinterpretResourceCast(const ShaderResourceView<FromFormat, ViewDimension>& srcSRV)
		{
			ShaderResourceView<ToFormat, ViewDimension> castResultSRV{};
			castResultSRV.mResourcePtr = srcSRV.mResourcePtr;
			castResultSRV.mViewDesc = srcSRV.mViewDesc;

			return castResultSRV;
		}

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_SRV_DIMENSION ViewDimension>
			requires (!Util::D3D12::IsSRVRTVDSVResourceCastLegal(FromFormat, ToFormat))
		ShaderResourceView<ToFormat, ViewDimension> ReinterpretResourceCast(const ShaderResourceView<FromFormat, ViewDimension>&)
		{
			static_assert(sizeof(ToFormat) != sizeof(ToFormat), "ERROR: An attempt was made to cast a resource to a different DXGI_FORMAT, but this cast is considered illegal! (Don't bother looking online for the casting rules. Just check the comments in D3D12UtilFormats.ixx.)");
		}
	}
}