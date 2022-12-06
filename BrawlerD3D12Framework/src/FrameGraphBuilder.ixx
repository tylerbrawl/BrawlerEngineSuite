module;
#include <vector>
#include <memory>

export module Brawler.D3D12.FrameGraphBuilder;
import Brawler.D3D12.RenderPass;
import Brawler.D3D12.RenderPassBundle;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUResourceLifetimeType;
import Brawler.SortedVector;

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraph;
		class I_RenderModule;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBuilder
		{
		private:
			friend class FrameGraph;
			friend class I_RenderModule;

		public:
			FrameGraphBuilder() = delete;
			explicit FrameGraphBuilder(FrameGraph& frameGraph);

			FrameGraphBuilder(const FrameGraphBuilder& rhs) = delete;
			FrameGraphBuilder& operator=(const FrameGraphBuilder& rhs) = delete;

			FrameGraphBuilder(FrameGraphBuilder&& rhs) noexcept = default;
			FrameGraphBuilder& operator=(FrameGraphBuilder&& rhs) noexcept = default;

			template <typename InputDataType>
			void AddDirectRenderPass(RenderPass<GPUCommandQueueType::DIRECT, InputDataType>&& renderPass);

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

			std::size_t GetRenderPassBundleCount() const;

			FrameGraph& GetFrameGraph();
			const FrameGraph& GetFrameGraph() const;

			std::span<RenderPassBundle> GetRenderPassBundleSpan();

			void MergeFrameGraphBuilder(FrameGraphBuilder&& mergedBuilder);

		private:
			std::vector<std::unique_ptr<I_GPUResource>> ExtractTransientResources();
			void SetRenderPassBundleIDs(std::uint32_t baseID);

			void UpdateTransientGPUResourceAliasTracker(TransientGPUResourceAliasTracker& aliasTracker) const;

			/// <summary>
			/// Creates the necessary sync points for the RenderPassBundle instances contained
			/// in this FrameGraphBuilder instance. These are necessary for setting resources
			/// shared across queues to be in the proper combined read state before the relevant
			/// commands are executed on the GPU.
			/// 
			/// In addition, sync points are also responsible for transitioning resources which
			/// are to be used in the copy queue to the D3D12_RESOURCE_STATE_COMMON state before
			/// they are used there.
			/// </summary>
			void CreateSyncPoints();

			void UpdateResourceUsageForCurrentFrame();
			Brawler::SortedVector<I_GPUResource*> ExtractResourceDependencyCache();

		private:
			std::vector<RenderPassBundle> mRenderPassBundleArr;
			std::vector<std::unique_ptr<I_GPUResource>> mTransientResourceArr;
			Brawler::SortedVector<I_GPUResource*> mResourceDependencyCache;
			FrameGraph* mFrameGraphPtr;
		};
	}
}

// ----------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename InputDataType>
		void FrameGraphBuilder::AddDirectRenderPass(RenderPass<GPUCommandQueueType::DIRECT, InputDataType>&& renderPass)
		{
			RenderPassBundle bundle{};
			bundle.AddDirectRenderPass(std::move(renderPass));

			mRenderPassBundleArr.push_back(std::move(bundle));
		}

		template <typename ResourceType, typename... Args>
			requires std::derived_from<ResourceType, I_GPUResource>
		ResourceType& FrameGraphBuilder::CreateTransientResource(Args&&... args)
		{
			std::unique_ptr<ResourceType> resource{ std::make_unique<ResourceType>(std::forward<Args>(args)...) };

			// Mark the resource as transient.
			resource->SetGPUResourceLifetimeType(GPUResourceLifetimeType::TRANSIENT);

			ResourceType* const resourcePtr = resource.get();
			mTransientResourceArr.push_back(std::move(resource));

			return *resourcePtr;
		}
	}
}