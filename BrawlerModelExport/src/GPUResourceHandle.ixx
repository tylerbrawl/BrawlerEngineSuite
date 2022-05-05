module;
#include <cassert>
#include <utility>

export module Brawler.D3D12.GPUResourceHandle;
import Brawler.D3D12.GPUResourceAccessMode;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
		class GPUJobGroup;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceAccessMode CurrentAccessMode, GPUResourceAccessMode RequiredAccessMode>
		concept IsLegalResourceAccess = (std::to_underlying(CurrentAccessMode) >= std::to_underlying(RequiredAccessMode));

		template <GPUResourceAccessMode AccessMode>
		class GPUResourceHandle
		{
		private:
			friend class GPUJobGroup;

			friend class GPUResourceHandle<GPUResourceAccessMode::READ_ONLY>;
			friend class GPUResourceHandle<GPUResourceAccessMode::READ_WRITE>;

		public:
			GPUResourceHandle() = default;

		private:
			explicit GPUResourceHandle(I_GPUResource& resource);

		public:
			// We want to be able to create resource handles by copying/moving resource handles
			// with equivalent or stricter access rights, but not greater. This is why we delete
			// some copy/move constructors/assignment operators.

			template <GPUResourceAccessMode RHSAccessMode>
				requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle(const GPUResourceHandle<RHSAccessMode>& rhs);

			template <GPUResourceAccessMode RHSAccessMode>
				requires !IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle(const GPUResourceHandle<RHSAccessMode>& rhs) = delete;

			template <GPUResourceAccessMode RHSAccessMode>
				requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle& operator=(const GPUResourceHandle<RHSAccessMode>& rhs);

			template <GPUResourceAccessMode RHSAccessMode>
				requires !IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle& operator=(const GPUResourceHandle<RHSAccessMode>& rhs) = delete;

			template <GPUResourceAccessMode RHSAccessMode>
				requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle(GPUResourceHandle<RHSAccessMode>&& rhs) noexcept;

			template <GPUResourceAccessMode RHSAccessMode>
				requires !IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle(GPUResourceHandle<RHSAccessMode>&& rhs) noexcept = delete;

			template <GPUResourceAccessMode RHSAccessMode>
				requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle& operator=(GPUResourceHandle<RHSAccessMode>&& rhs) noexcept;

			template <GPUResourceAccessMode RHSAccessMode>
				requires !IsLegalResourceAccess<RHSAccessMode, AccessMode>
			GPUResourceHandle& operator=(GPUResourceHandle<RHSAccessMode>&& rhs) noexcept = delete;

			constexpr GPUResourceAccessMode GetAccessMode() const;

			I_GPUResource& operator*() const;

			I_GPUResource* operator->() const;

		private:
			I_GPUResource* mResourcePtr;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceAccessMode AccessMode>
		GPUResourceHandle<AccessMode>::GPUResourceHandle(I_GPUResource& resource) :
			mResourcePtr(&resource)
		{}

		template <GPUResourceAccessMode AccessMode>
		template <GPUResourceAccessMode RHSAccessMode>
			requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
		GPUResourceHandle<AccessMode>::GPUResourceHandle(const GPUResourceHandle<RHSAccessMode>& rhs) :
			mResourcePtr(rhs.mResourcePtr)
		{}

		template <GPUResourceAccessMode AccessMode>
		template <GPUResourceAccessMode RHSAccessMode>
			requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
		GPUResourceHandle<AccessMode>& GPUResourceHandle<AccessMode>::operator=(const GPUResourceHandle<RHSAccessMode>& rhs)
		{
			mResourcePtr = rhs.mResourcePtr;

			return *this;
		}

		template <GPUResourceAccessMode AccessMode>
		template <GPUResourceAccessMode RHSAccessMode>
			requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
		GPUResourceHandle<AccessMode>::GPUResourceHandle(GPUResourceHandle<RHSAccessMode>&& rhs) noexcept :
			mResourcePtr(rhs.mResourcePtr)
		{
			rhs.mResourcePtr = nullptr;
		}

		template <GPUResourceAccessMode AccessMode>
		template <GPUResourceAccessMode RHSAccessMode>
			requires IsLegalResourceAccess<RHSAccessMode, AccessMode>
		GPUResourceHandle<AccessMode>& GPUResourceHandle<AccessMode>::operator=(GPUResourceHandle<RHSAccessMode>&& rhs) noexcept
		{
			mResourcePtr = rhs.mResourcePtr;
			rhs.mResourcePtr = nullptr;

			return *this;
		}

		template <GPUResourceAccessMode AccessMode>
		constexpr GPUResourceAccessMode GPUResourceHandle<AccessMode>::GetAccessMode() const
		{
			return AccessMode;
		}

		template <GPUResourceAccessMode AccessMode>
		I_GPUResource& GPUResourceHandle<AccessMode>::operator*() const
		{
			assert(mResourcePtr != nullptr && "ERROR: An attempt was made to access a nullptr GPUResourceHandle! (Did you previously move from it?)");
			return *mResourcePtr;
		}

		template <GPUResourceAccessMode AccessMode>
		I_GPUResource* GPUResourceHandle<AccessMode>::operator->() const
		{
			assert(mResourcePtr != nullptr && "ERROR: An attempt was made to access a nullptr GPUResourceHandle! (Did you previously move from it?)");
			return mResourcePtr;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using GPUResourceReadHandle = GPUResourceHandle<GPUResourceAccessMode::READ_ONLY>;
		using GPUResourceWriteHandle = GPUResourceHandle<GPUResourceAccessMode::READ_WRITE>;
	}
}