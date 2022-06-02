module;
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.UnorderedAccessView;
import Brawler.D3D12.I_GPUResource;
import Util.D3D12;
import Brawler.OptionalRef;
import Brawler.D3D12.UAVCounterSubAllocation;

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_UAV_DIMENSION ViewDimension>
		struct ViewDimensionInfo
		{
			static_assert(sizeof(ViewDimension) != sizeof(ViewDimension));
		};

		template <>
		struct ViewDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>
		{
			using ViewDescType = D3D12_BUFFER_UAV;

			static constexpr __forceinline void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const ViewDescType& viewDesc)
			{
				uavDesc.Buffer = viewDesc;
			}
		};

		template <>
		struct ViewDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D>
		{
			using ViewDescType = D3D12_TEX1D_UAV;

			static constexpr __forceinline void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const ViewDescType& viewDesc)
			{
				uavDesc.Texture1D = viewDesc;
			}
		};

		template <>
		struct ViewDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY>
		{
			using ViewDescType = D3D12_TEX1D_ARRAY_UAV;

			static constexpr __forceinline void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const ViewDescType& viewDesc)
			{
				uavDesc.Texture1DArray = viewDesc;
			}
		};

		template <>
		struct ViewDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>
		{
			using ViewDescType = D3D12_TEX2D_UAV;

			static constexpr __forceinline void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const ViewDescType& viewDesc)
			{
				uavDesc.Texture2D = viewDesc;
			}
		};

		template <>
		struct ViewDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY>
		{
			using ViewDescType = D3D12_TEX2D_ARRAY_UAV;

			static constexpr __forceinline void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const ViewDescType& viewDesc)
			{
				uavDesc.Texture2DArray = viewDesc;
			}
		};

		template <>
		struct ViewDimensionInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D>
		{
			using ViewDescType = D3D12_TEX3D_UAV;

			static constexpr __forceinline void InitializeUAVDescription(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, const ViewDescType& viewDesc)
			{
				uavDesc.Texture3D = viewDesc;
			}
		};

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		class UnorderedAccessView;

		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_UAV_DIMENSION ViewDimension>
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

			consteval Brawler::OptionalRef<const UAVCounterSubAllocation> GetUAVCounterSubAllocation() const
			{
				return Brawler::OptionalRef<const UAVCounterSubAllocation>{};
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

			void SetUAVCounterSubAllocation(const UAVCounterSubAllocation& uavCounter)
			{
				assert(uavCounter.HasReservation() && "ERROR: An attempt was made to provide an UnorderedAccessView with a UAV counter, but this counter was never given a reservation within a BufferResource!");
				mUAVCounterPtr = &uavCounter;
			}

			Brawler::OptionalRef<const UAVCounterSubAllocation> GetUAVCounterSubAllocation() const
			{
				if (mUAVCounterPtr == nullptr)
					return Brawler::OptionalRef<const UAVCounterSubAllocation>{};

				return Brawler::OptionalRef<const UAVCounterSubAllocation>{ *mUAVCounterPtr };
			}

		private:
			const UAVCounterSubAllocation* mUAVCounterPtr;
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
			using ViewDescType = typename ViewDimensionInfo<ViewDimension>::ViewDescType;
			
			static constexpr bool ALLOW_UAV_COUNTER = UAVCounterContainer<Format, ViewDimension>::IsUAVCounterValid();

		private:
			template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat>
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

			void SetUAVCounter(const UAVCounterSubAllocation& uavCounter) requires ALLOW_UAV_COUNTER;
			Brawler::OptionalRef<const UAVCounterSubAllocation> GetUAVCounter() const;

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
		void UnorderedAccessView<Format, ViewDimension>::SetUAVCounter(const UAVCounterSubAllocation& uavCounter) requires ALLOW_UAV_COUNTER
		{
			this->SetUAVCounterSubAllocation(uavCounter);
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		Brawler::OptionalRef<const UAVCounterSubAllocation> UnorderedAccessView<Format, ViewDimension>::GetUAVCounter() const
		{
			if constexpr (ALLOW_UAV_COUNTER)
				return this->GetUAVCounterSubAllocation();

			else
				return Brawler::OptionalRef<const UAVCounterSubAllocation>{};
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

			ViewDimensionInfo<ViewDimension>::InitializeUAVDescription(uavDesc, mViewDesc);

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
		template <DXGI_FORMAT ToFormat, DXGI_FORMAT FromFormat, D3D12_UAV_DIMENSION ViewDimension>
		UnorderedAccessView<ToFormat, ViewDimension> ReinterpretResourceCast(const UnorderedAccessView<FromFormat, ViewDimension>& srcUav)
		{
			static_assert(Util::D3D12::IsUAVResourceCastLegal(FromFormat, ToFormat), "ERROR: An attempt was made to cast a resource to a different DXGI_FORMAT, but this cast is considered illegal! (Don't bother looking online for the casting rules. Just check the comments in D3D12UtilFormats.ixx.)");

			UnorderedAccessView<ToFormat, ViewDimension> destUav{};
			destUav.mOwningResourcePtr = srcUav.mOwningResourcePtr;
			destUav.mViewDesc = srcUav.mViewDesc;

			return destUav;
		}
	}
}