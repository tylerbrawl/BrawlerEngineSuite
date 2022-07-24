module;
#include <memory>
#include <shared_mutex>
#include <vector>
#include <cassert>
#include <ranges>
#include "DxDef.h"

module Brawler.D3D12.BindlessGPUResourceGroup;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		BindlessGPUResourceGroup::~BindlessGPUResourceGroup()
		{
			// In Debug builds, fire an assert if an I_GPUResource instance was somehow destroyed
			// before the BindlessGPUResourceGroup instance which it was associated with was destroyed.
			//
			// This attempts to ensure that I_GPUResource instances do not access garbage later on.
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				bool doesValidRegistrationExist = false;

				mRegistrationInfoArr.ForEach([&doesValidRegistrationExist] (const RegistrationInfo& info)
				{
					if (info.RegistrationPtr->IsValid()) [[unlikely]]
						doesValidRegistrationExist = true;
				});

				assert(!doesValidRegistrationExist && "ERROR: A BindlessGPUResourceGroup instance was destroyed before all of the I_GPUResource instances which were associated with it either were destroyed themselves or had their registration revoked!");
			}
		}

		void BindlessGPUResourceGroup::RegisterGPUResource(I_GPUResource& resource)
		{
			RegistrationInfo registrationInfo{
				.ResourcePtr = &resource,
				.RegistrationPtr{ std::make_unique<BindlessGPUResourceGroupRegistration>() }
			};

			BindlessGPUResourceGroupRegistration* const rawRegistrationPtr = registrationInfo.RegistrationPtr.get();

			// Add a unique RegistrationInfo instance for each I_GPUResource instance. In Debug builds,
			// we fire an assert if more than one RegistrationInfo instance exists for a given I_GPUResource
			// instance.
			//
			// In order to make the search thread safe, we do it *after* adding registrationInfo.
			mRegistrationInfoArr.PushBack(std::move(registrationInfo));

			if constexpr (Util::General::IsDebugModeEnabled())
			{
				std::size_t currResourceCount = 0;

				mRegistrationInfoArr.ForEach([resourcePtr = &resource, &currResourceCount] (const RegistrationInfo& info)
				{
					if (info.ResourcePtr == resourcePtr && info.RegistrationPtr->IsValid()) [[unlikely]]
						++currResourceCount;
				});

				assert(currResourceCount == 1 && "ERROR: An attempt was made to add the same I_GPUResource instance to a BindlessGPUResourceGroup more than once!");
			}

			resource.AddBindlessGPUResourceGroupAssociation(*rawRegistrationPtr);
		}

		void BindlessGPUResourceGroup::UnregisterGPUResource(I_GPUResource& resource)
		{
			// Find the RegistrationInfo instance which describes this I_GPUResource instance. If it
			// exists, then we merely inform resource to remove the registration, rather than erase the
			// entry entirely from mRegistrationInfoArr.
			//
			// This prevents the call to BindlessGPUResourceGroupRegistration::MarkAsInvalidated() made
			// on behalf of resource from accessing garbage.
			BindlessGPUResourceGroupRegistration* registrationPtr = nullptr;

			mRegistrationInfoArr.ForEach([&registrationPtr, resourcePtr = &resource] (const RegistrationInfo& info)
			{
				if (info.ResourcePtr == resourcePtr) [[unlikely]]
				{
					assert(registrationPtr == nullptr && "ERROR: An attempt was made to add the same I_GPUResource instance to a BindlessGPUResourceGroup more than once!");
					registrationPtr = info.RegistrationPtr.get();
				}
			});

			if (registrationPtr != nullptr) [[likely]]
				resource.RemoveBindlessGPUResourceGroupAssociation(*registrationPtr);
		}

		std::vector<FrameGraphResourceDependency> BindlessGPUResourceGroup::CreateResourceDependencyArray(const D3D12_RESOURCE_STATES requiredState)
		{
			std::vector<FrameGraphResourceDependency> resourceDependencyArr{};
			resourceDependencyArr.reserve(mRegistrationInfoArr.GetSize());

			// If any of the BindlessGPUResourceGroupRegistration instances were marked as invalid, then
			// either the corresponding I_GPUResource instance was destroyed or it no longer wishes to
			// be associated with this BindlessGPUResourceGroup. Either way, we should remove the
			// RegistrationInfo instance from mRegistrationInfoArr.
			bool isRegistrationInfoDirty = false;

			mRegistrationInfoArr.ForEach([&isRegistrationInfoDirty, &resourceDependencyArr, requiredState] (const RegistrationInfo& info)
			{
				// Strictly speaking, there is a chance that the I_GPUResource instance is destroyed on
				// another thread while we are attempting to access it here. I argue, however, that this
				// would be the API user's fault for not ensuring that the I_GPUResource instance remained
				// alive while FrameGraphResourceDependency instances were being created.
				//
				// This is because FrameGraph building assumes that I_GPUResource instances will remain
				// alive; internally, FrameGraphResourceDependency instances have always cached I_GPUResource*
				// values.
				if (!info.RegistrationPtr->IsValid()) [[unlikely]]
				{
					isRegistrationInfoDirty = true;
					return;
				}

				const std::uint32_t subResourceCount = info.ResourcePtr->GetSubResourceCount();

				for (const auto i : std::views::iota(0u, subResourceCount))
					resourceDependencyArr.push_back(FrameGraphResourceDependency{
						.ResourcePtr = info.ResourcePtr,
						.RequiredState = requiredState,
						.SubResourceIndex = i
					});
			});

			// If we found that the array contained invalid entries, then remove these. If multiple threads
			// are calling this function concurrently, then they may all have isRegistrationInfoDirty == true,
			// even though only one thread might actually make any real changes. I have yet to see if this would
			// cause any significant performance problems.
			if (isRegistrationInfoDirty) [[unlikely]]
			{
				mRegistrationInfoArr.EraseIf([] (const RegistrationInfo& info)
				{
					return (!info.RegistrationPtr->IsValid());
				});
			}

			return resourceDependencyArr;
		}
	}
}