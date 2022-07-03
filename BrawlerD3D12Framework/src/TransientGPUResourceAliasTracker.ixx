module;
#include <span>
#include <unordered_map>
#include <vector>
#include <optional>

export module Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.I_GPUResourceHeapManager;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class RenderPassBundle;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class TransientGPUResourceAliasTracker
		{
		private:
			struct TransientGPUResourceInfo
			{
				I_GPUResource* ResourcePtr;
				std::uint64_t ResourceSize;

				/// <summary>
				/// This is the identifier of the first RenderPassBundle which used the corresponding
				/// transient resource.
				/// </summary>
				std::uint32_t FirstBundleUsage;

				/// <summary>
				/// This is the identifier of the last RenderPassBundle which used the corresponding
				/// transient resource.
				/// </summary>
				std::uint32_t LastBundleUsage;

				/// <summary>
				/// This value correlates resources which belong to the same region of GPU memory
				/// together. This correlation can indeed be assumed by analyzing the return value of
				/// TransientGPUResourceAliasTracker::GetAliasableResources(), but having a separate
				/// value allows for a more efficient method of checking.
				/// </summary>
				std::uint32_t AliasGroupID;

				bool Overlaps(const TransientGPUResourceInfo& otherInfo) const;
			};

		public:
			TransientGPUResourceAliasTracker() = default;

			TransientGPUResourceAliasTracker(const TransientGPUResourceAliasTracker& rhs) = delete;
			TransientGPUResourceAliasTracker& operator=(const TransientGPUResourceAliasTracker& rhs) = delete;

			TransientGPUResourceAliasTracker(TransientGPUResourceAliasTracker&& rhs) noexcept = default;
			TransientGPUResourceAliasTracker& operator=(TransientGPUResourceAliasTracker&& rhs) noexcept = default;

			void SetTransientGPUResourceHeapManager(const I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>& resourceHeapManager);

			void AddTransientResourceDependencyForBundle(const std::uint32_t bundleID, I_GPUResource& transientResourceDependency);
			void CalculateAliasableResources();

			std::span<const std::vector<I_GPUResource*>> GetAliasableResources() const;

			std::optional<std::uint32_t> GetAliasGroupID(const I_GPUResource& resource) const;
			std::size_t GetAliasGroupCount() const;

		private:
			const I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>* mResourceHeapManagerPtr;
			std::unordered_map<I_GPUResource*, TransientGPUResourceInfo> mResourceLifetimeMap;
			std::vector<std::vector<I_GPUResource*>> mAliasableResourcesArr;
		};
	}
}