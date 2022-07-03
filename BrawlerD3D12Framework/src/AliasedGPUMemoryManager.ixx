module;
#include <vector>

export module Brawler.D3D12.AliasedGPUMemoryManager;

export namespace Brawler
{
	namespace D3D12
	{
		class TransientGPUResourceAliasTracker;
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class AliasedGPUMemoryManager
		{
		private:
			struct AliasedGPUMemoryState
			{
				const I_GPUResource* CurrentResourcePtr;
			};

		public:
			explicit AliasedGPUMemoryManager(const TransientGPUResourceAliasTracker& aliasTracker);

			AliasedGPUMemoryManager(const AliasedGPUMemoryManager& rhs) = default;
			AliasedGPUMemoryManager& operator=(const AliasedGPUMemoryManager& rhs) = default;

			AliasedGPUMemoryManager(AliasedGPUMemoryManager&& rhs) noexcept = default;
			AliasedGPUMemoryManager& operator=(AliasedGPUMemoryManager&& rhs) noexcept = default;

			/// <summary>
			/// Sets the specified I_GPUResource instance as the active GPU resource for its
			/// aliased memory region. If a different I_GPUResource already held this memory
			/// region, then the returned I_GPUResource* is a pointer to this previous resource;
			/// otherwise, the returned value is nullptr.
			/// </summary>
			/// <param name="activatedResource">
			/// - The I_GPUResource which is to be activated in its memory region.
			/// </param>
			/// <returns>
			/// If an aliasing barrier is needed as a result of activating this I_GPUResource,
			/// then the returned pointer should be the ResourceBefore of said aliasing barrier;
			/// otherwise, the function returns nullptr, and no aliasing barrier is necessary to
			/// make use of the I_GPUResource.
			/// </returns>
			const I_GPUResource* ActivateGPUResource(const I_GPUResource& activatedResource);

		private:
			const TransientGPUResourceAliasTracker* mAliasTrackerPtr;
			std::vector<AliasedGPUMemoryState> mStateArr;
		};
	}
}