module;
#include <memory>
#include <shared_mutex>
#include <vector>
#include "DxDef.h"

export module Brawler.D3D12.BindlessGPUResourceGroup;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.ThreadSafeVector;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.BindlessGPUResourceGroupRegistration;

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessGPUResourceGroup
		{
		private:
			struct RegistrationInfo
			{
				I_GPUResource* ResourcePtr;
				std::unique_ptr<BindlessGPUResourceGroupRegistration> RegistrationPtr;
			};

		public:
			BindlessGPUResourceGroup() = default;

			~BindlessGPUResourceGroup();

			BindlessGPUResourceGroup(const BindlessGPUResourceGroup& rhs) = delete;
			BindlessGPUResourceGroup& operator=(const BindlessGPUResourceGroup& rhs) = delete;

			BindlessGPUResourceGroup(BindlessGPUResourceGroup&& rhs) noexcept = default;
			BindlessGPUResourceGroup& operator=(BindlessGPUResourceGroup&& rhs) noexcept = default;

			void RegisterGPUResource(I_GPUResource& resource);
			void UnregisterGPUResource(I_GPUResource& resource);

			std::vector<FrameGraphResourceDependency> CreateResourceDependencyArray(const D3D12_RESOURCE_STATES requiredState);

		private:
			Brawler::ThreadSafeVector<RegistrationInfo, std::shared_mutex> mRegistrationInfoArr;
		};
	}
}