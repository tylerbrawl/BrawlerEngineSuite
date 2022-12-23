module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.UnorderedAccessView;
import :UAVDimensionInfo;
import Brawler.D3D12.I_GPUResource;
import Util.D3D12;
import Brawler.OptionalRef;
import Brawler.D3D12.UAVCounterSubAllocation;

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		class UnorderedAccessView;

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_UAV_DIMENSION ViewDimension>
			requires (Util::D3D12::IsUAVResourceCastLegal(FromFormat, ToFormat))
		UnorderedAccessView<ToFormat, ViewDimension> ReinterpretResourceCast(const UnorderedAccessView<FromFormat, ViewDimension>& srcUav);

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		class UAVCounterContainer
		{
		public:
			UAVCounterContainer() = default;
			virtual ~UAVCounterContainer() = default;

			static consteval bool IsUAVCounterValid()
			{
				return false;
			}

			consteval Brawler::OptionalRef<Brawler::D3D12Resource> GetUAVCounterResource() const
			{
				return Brawler::OptionalRef<Brawler::D3D12Resource>{};
			}
		};

		template <>
		class UAVCounterContainer<DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>
		{
		public:
			UAVCounterContainer() = default;
			virtual ~UAVCounterContainer() = default;

			static consteval bool IsUAVCounterValid()
			{
				return true;
			}

			void SetUAVCounterResource(Brawler::D3D12Resource& uavCounterResource)
			{
				mUAVCounterResourcePtr = &uavCounterResource;
			}

			Brawler::OptionalRef<Brawler::D3D12Resource> GetUAVCounterResource() const
			{
				if (mUAVCounterResourcePtr == nullptr) [[likely]]
					return Brawler::OptionalRef<Brawler::D3D12Resource>{};

				return Brawler::OptionalRef<Brawler::D3D12Resource>{ *mUAVCounterResourcePtr };
			}

		private:
			Brawler::D3D12Resource* mUAVCounterResourcePtr;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		class UnorderedAccessView final : private UAVCounterContainer<Format, ViewDimension>
		{
		private:
			using ViewDescType = typename UAVDimensionInfo<ViewDimension>::UAVDescType;
			
			static constexpr bool ALLOW_UAV_COUNTER = UAVCounterContainer<Format, ViewDimension>::IsUAVCounterValid();

		private:
			template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat>
				requires (Util::D3D12::IsUAVResourceCastLegal(FromFormat, ToFormat))
			friend UnorderedAccessView<ToFormat, ViewDimension> ReinterpretResourceCast(const UnorderedAccessView<FromFormat, ViewDimension>& srcUav);

		public:
			UnorderedAccessView() = default;
			UnorderedAccessView(const I_GPUResource& resource, ViewDescType&& viewDesc);

			UnorderedAccessView(const UnorderedAccessView& rhs) = default;
			UnorderedAccessView& operator=(const UnorderedAccessView& rhs) = default;

			UnorderedAccessView(UnorderedAccessView&& rhs) noexcept = default;
			UnorderedAccessView& operator=(UnorderedAccessView&& rhs) noexcept = default;

			const I_GPUResource& GetGPUResource() const;
			Brawler::D3D12Resource& GetD3D12Resource() const;

			void SetUAVCounterResource(Brawler::D3D12Resource& uavCounterResource) requires ALLOW_UAV_COUNTER
			{
				UAVCounterContainer<Format, ViewDimension>::SetUAVCounterResource(uavCounterResource);
			}

			Brawler::OptionalRef<Brawler::D3D12Resource> GetUAVCounterResource() const;

			D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUAVDescription() const;

		private:
			const I_GPUResource* mOwningResourcePtr;
			ViewDescType mViewDesc;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		UnorderedAccessView<Format, ViewDimension>::UnorderedAccessView(const I_GPUResource& resource, ViewDescType&& viewDesc) :
			mOwningResourcePtr(&resource),
			mViewDesc(std::move(viewDesc))
		{
			assert(Util::D3D12::IsUAVResourceCastLegal(resource.GetResourceDescription().Format, Format) && "ERROR: An attempt was made to create an UnorderedAccessView with a different format than that of the I_GPUResource which it was supposed to represent, but the cast from the resource's format to that of the UAV was illegal!");
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		const I_GPUResource& UnorderedAccessView<Format, ViewDimension>::GetGPUResource() const
		{
			assert(mOwningResourcePtr != nullptr);
			return *mOwningResourcePtr;
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		Brawler::D3D12Resource& UnorderedAccessView<Format, ViewDimension>::GetD3D12Resource() const
		{
			assert(mOwningResourcePtr != nullptr);
			return (mOwningResourcePtr->GetD3D12Resource());
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		Brawler::OptionalRef<Brawler::D3D12Resource> UnorderedAccessView<Format, ViewDimension>::GetUAVCounterResource() const
		{
			if constexpr (ALLOW_UAV_COUNTER)
				return UAVCounterContainer<Format, ViewDimension>::GetUAVCounterResource();

			else
				return Brawler::OptionalRef<Brawler::D3D12Resource>{};
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessView<Format, ViewDimension>::CreateUAVDescription() const
		{
			// We're also getting weird errors when we try to construct a D3D12_UNORDERED_ACCESS_VIEW_DESC
			// instance, much like we do with ShaderResourceView::CreateSRVDescription(). Since we're just going
			// to be initializing all of the fields anyways, however, it doesn't matter too much.
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = Format;
			uavDesc.ViewDimension = ViewDimension;

			InitializeUAVDescription<ViewDimension>(uavDesc, mViewDesc);

			return uavDesc;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <DXGI_FORMAT Format>
			requires (Format != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
		using TypedBufferUnorderedAccessView = UnorderedAccessView<Format, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>;

		using StructuredBufferUnorderedAccessView = UnorderedAccessView<DXGI_FORMAT::DXGI_FORMAT_UNKNOWN, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>;
		using RawBufferUnorderedAccessView = UnorderedAccessView<DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>;

		template <DXGI_FORMAT Format>
		using Texture1DUnorderedAccessView = UnorderedAccessView<Format, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D>;

		template <DXGI_FORMAT Format>
		using Texture1DArrayUnorderedAccessView = UnorderedAccessView<Format, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY>;

		template <DXGI_FORMAT Format>
		using Texture2DUnorderedAccessView = UnorderedAccessView<Format, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>;

		template <DXGI_FORMAT Format>
		using Texture2DArrayUnorderedAccessView = UnorderedAccessView<Format, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY>;

		template <DXGI_FORMAT Format>
		using Texture3DUnorderedAccessView = UnorderedAccessView<Format, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D>;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		// An internal compiler error (ICE) in the MSVC is preventing us from doing a static_assert on the value
		// of Util::D3D12::IsUAVResourceCastLegal(). To get the original behavior, we now define the function
		// twice, but with different constraints. If the resource cast is legal, then the original function is
		// used; otherwise, a dummy function is used which does a static_assert that always fires when triggered.

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_UAV_DIMENSION ViewDimension>
			requires (Util::D3D12::IsUAVResourceCastLegal(FromFormat, ToFormat))
		UnorderedAccessView<ToFormat, ViewDimension> ReinterpretResourceCast(const UnorderedAccessView<FromFormat, ViewDimension>& srcUav)
		{
			UnorderedAccessView<ToFormat, ViewDimension> destUav{};
			destUav.mOwningResourcePtr = srcUav.mOwningResourcePtr;
			destUav.mViewDesc = srcUav.mViewDesc;

			return destUav;
		}

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_UAV_DIMENSION ViewDimension>
			requires (!Util::D3D12::IsUAVResourceCastLegal(FromFormat, ToFormat))
		UnorderedAccessView<ToFormat, ViewDimension> ReinterpretResourceCast(const UnorderedAccessView<FromFormat, ViewDimension>& srcUav)
		{
			static_assert(sizeof(ToFormat) != sizeof(ToFormat), "ERROR: An attempt was made to cast a resource to a different DXGI_FORMAT, but this cast is considered illegal! (Don't bother looking online for the casting rules. Just check the comments in D3D12UtilFormats.ixx.)");
		}
	}
}