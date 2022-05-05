module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.ScopedGPUResourceMapping;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename PointerT = void>
		class ScopedGPUResourceMapping
		{
		public:
			enum class GenericReadRange
			{
				/// <summary>
				/// This value indicates that the CPU will not be reading any data from this resource. When applicable,
				/// you should specify this option to improve performance.
				/// </summary>
				DISABLE_READING,

				/// <summary>
				/// This value indicates that the CPU will read the entire subresource.
				/// </summary>
				READ_ENTIRE_SUBRESOURCE
			};

		private:
			using UnderlyingPointer_T = std::remove_pointer_t<std::decay_t<PointerT>>;

		public:
			/// <summary>
			/// Constructs a ScopedGPUResourceMapping for the specified I_GPUResource. When the ScopedGPUResourceMapping
			/// is destroyed, the resource (or, to be more precise, its relevant subresource) is unmapped.
			/// </summary>
			/// <param name="resource">
			/// - The resource for which a mapping is to be made for CPU access.
			/// </param>
			/// <param name="readableRange">
			/// - A value from the ScopedGPUResourceMapping::GenericReadRange enumeration which specifies what parts of
			///   the resource will be read in this mapping.
			/// </param>
			/// <param name="subresourceIndex">
			/// - The index of the subresource within the I_GPUResource which is to be mapped. By default, this value is
			///   set to 0.
			/// </param>
			ScopedGPUResourceMapping(I_GPUResource& resource, const GenericReadRange readableRange, const std::uint32_t subresourceIndex = 0);

			/// <summary>
			/// Constructs a ScopedGPUResourceMapping for the specified I_GPUResource. When the ScopedGPUResourceMapping
			/// is destroyed, the resource (or, to be more precise, its relevant subresource) is unmapped.
			/// </summary>
			/// <param name="resource">
			/// - The resource for which a mapping is to be made for CPU access.
			/// </param>
			/// <param name="readRange">
			/// - The range of memory which the CPU is allowed to read. If the CPU does not need to read any data, then you 
			///   should use the other constructor with readableRange parameter set to 
			///   ScopedGPUResourceMapping::GenericReadRange::DISABLE_READING. This can prevent unnecessary and expensive 
			///   cache flushes caused by reading data from a resource allocated on an upload heap.
			/// </param>
			/// <param name="subresourceIndex">
			/// - The index of the subresource within the I_GPUResource which is to be mapped. By default, this value is
			///   set to 0.
			/// </param>
			ScopedGPUResourceMapping(I_GPUResource& resource, const D3D12_RANGE& readRange, const std::uint32_t subresourceIndex = 0);

			~ScopedGPUResourceMapping();

			ScopedGPUResourceMapping(const ScopedGPUResourceMapping& rhs) = delete;
			ScopedGPUResourceMapping& operator=(const ScopedGPUResourceMapping& rhs) = delete;

			ScopedGPUResourceMapping(ScopedGPUResourceMapping&& rhs) noexcept;
			ScopedGPUResourceMapping& operator=(ScopedGPUResourceMapping&& rhs) noexcept;

			UnderlyingPointer_T* Get();
			const UnderlyingPointer_T* Get() const;

		private:
			void UnmapResource();

		private:
			Brawler::D3D12Resource* mResourcePtr;
			std::uint32_t mSubresourceIndex;
			UnderlyingPointer_T* mDataPtr;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename PointerT>
		ScopedGPUResourceMapping<PointerT>::ScopedGPUResourceMapping(I_GPUResource& resource, const GenericReadRange readableRange, const std::uint32_t subresourceIndex) :
			mResourcePtr(&(resource.GetD3D12Resource())),
			mSubresourceIndex(subresourceIndex),
			mDataPtr(nullptr)
		{
			static constexpr D3D12_RANGE DISABLE_READING_RANGE{ 0, 0 };

			if (readableRange == GenericReadRange::DISABLE_READING)
			{
				CheckHRESULT(mResourcePtr->Map(subresourceIndex, &DISABLE_READING_RANGE, reinterpret_cast<void**>(&mDataPtr)));
			}
			else
			{
				CheckHRESULT(mResourcePtr->Map(subresourceIndex, nullptr, reinterpret_cast<void**>(&mDataPtr)));
			}
		}

		template <typename PointerT>
		ScopedGPUResourceMapping<PointerT>::ScopedGPUResourceMapping(I_GPUResource& resource, const D3D12_RANGE& readRange, const std::uint32_t subresourceIndex) :
			mResourcePtr(&(resource.GetD3D12Resource())),
			mSubresourceIndex(subresourceIndex),
			mDataPtr(nullptr)
		{
			CheckHRESULT(mResourcePtr->Map(subresourceIndex, &readRange, reinterpret_cast<void**>(&mDataPtr)));
		}

		template <typename PointerT>
		ScopedGPUResourceMapping<PointerT>::~ScopedGPUResourceMapping()
		{
			UnmapResource();
		}

		template <typename PointerT>
		ScopedGPUResourceMapping<PointerT>::ScopedGPUResourceMapping(ScopedGPUResourceMapping&& rhs) noexcept :
			mResourcePtr(rhs.mResourcePtr),
			mSubresourceIndex(rhs.mSubresourceIndex),
			mDataPtr(rhs.mDataPtr)
		{
			rhs.mResourcePtr = nullptr;
			rhs.mDataPtr = nullptr;
		}

		template <typename PointerT>
		ScopedGPUResourceMapping<PointerT>& ScopedGPUResourceMapping<PointerT>::operator=(ScopedGPUResourceMapping&& rhs) noexcept
		{
			UnmapResource();

			mResourcePtr = rhs.mResourcePtr;
			rhs.mResourcePtr = nullptr;

			mSubresourceIndex = rhs.mSubresourceIndex;

			mDataPtr = rhs.mDataPtr;
			rhs.mDataPtr = nullptr;

			return *this;
		}

		template <typename PointerT>
		ScopedGPUResourceMapping<PointerT>::UnderlyingPointer_T* ScopedGPUResourceMapping<PointerT>::Get()
		{
			return mDataPtr;
		}

		template <typename PointerT>
		const typename ScopedGPUResourceMapping<PointerT>::UnderlyingPointer_T* ScopedGPUResourceMapping<PointerT>::Get() const
		{
			return mDataPtr;
		}

		template <typename PointerT>
		void ScopedGPUResourceMapping<PointerT>::UnmapResource()
		{
			if (mResourcePtr != nullptr && mDataPtr != nullptr)
			{
				mResourcePtr->Unmap(mSubresourceIndex, nullptr);

				mResourcePtr = nullptr;
				mDataPtr = nullptr;
			}
		}
	}
}