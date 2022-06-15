module;
#include <vector>
#include <memory>
#include <span>

export module Brawler.D3D12.FrameGraphSubBuilder;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.RenderPassBundle;
import Brawler.D3D12.GPUResourceLifetimeType;

namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphSubBuilder
		{
		public:
			FrameGraphSubBuilder() = default;

			FrameGraphSubBuilder(const FrameGraphSubBuilder& rhs) = delete;
			FrameGraphSubBuilder& operator=(const FrameGraphSubBuilder& rhs) = delete;

			FrameGraphSubBuilder(FrameGraphSubBuilder&& rhs) noexcept = default;
			FrameGraphSubBuilder& operator=(FrameGraphSubBuilder&& rhs) noexcept = default;

			void AddRenderPassBundle(RenderPassBundle&& bundle);

			/// <summary>
			/// Creates a transient I_GPUResource of type ResourceType. The values provided in args...
			/// are passed to the constructor of ResourceType.
			/// 
			/// A transient resource is one whose memory allocation lasts only for the frame in which
			/// it is used. Transient resources are the only resources which can be aliased with each
			/// other in order to save GPU memory. Therefore, when it is possible to make a resource
			/// transient, you should do so.
			/// 
			/// 
			/// </summary>
			/// <typeparam name="ResourceType"></typeparam>
			/// <typeparam name="...Args"></typeparam>
			/// <param name="...args"></param>
			/// <returns></returns>
			template <typename ResourceType, typename... Args>
				requires std::derived_from<ResourceType, I_GPUResource>
			ResourceType& CreateTransientResource(Args&&... args);

			std::span<RenderPassBundle> GetRenderPassBundleSpan();
			std::span<std::unique_ptr<I_GPUResource>> GetTransientResourceSpan();

		private:
			std::vector<RenderPassBundle> mRenderPassBundleArr;
			std::vector<std::unique_ptr<I_GPUResource>> mTransientResourcePtrArr;
		};
	}
}

// -------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename ResourceType, typename... Args>
			requires std::derived_from<ResourceType, I_GPUResource>
		ResourceType& FrameGraphSubBuilder::CreateTransientResource(Args&&... args)
		{
			std::unique_ptr<ResourceType> resource{ std::make_unique<ResourceType>(std::forward<Args>(args)...) };

			// Mark the resource as transient.
			resource->SetGPUResourceLifetimeType(GPUResourceLifetimeType::TRANSIENT);

			ResourceType* const resourcePtr = resource.get();
			mTransientResourcePtrArr.push_back(std::move(resource));

			return *resourcePtr;
		}
	}
}