module;
#include <vector>
#include <memory>
#include <span>
#include <utility>
#include <optional>

export module Brawler.D3D12.FrameGraphCompilation:GPUExecutionModule;
import Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.SortedVector;
import Brawler.CompositeEnum;
import Brawler.D3D12.GPUCommandListRecorder;
import Brawler.D3D12.GPUExecutionModuleResourceMap;
import Brawler.D3D12.ResourceStateZone;

export namespace Brawler
{
	namespace D3D12
	{
		class RenderPassBundle;
		struct GPUExecutionModuleRecordContext;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUExecutionModule
		{
		private:
			template <GPUCommandQueueType QueueType>
			struct RenderPassContainer
			{
				std::vector<std::unique_ptr<I_RenderPass<QueueType>>> RenderPassArr;
				std::vector<std::unique_ptr<GPUCommandListRecorder<QueueType>>> CmdRecorderArr;
				GPUExecutionModuleResourceMap<QueueType> ResourceMap;
			};

		public:
			GPUExecutionModule();

			GPUExecutionModule(const GPUExecutionModule& rhs) = delete;
			GPUExecutionModule& operator=(const GPUExecutionModule& rhs) = delete;

			GPUExecutionModule(GPUExecutionModule&& rhs) noexcept = default;
			GPUExecutionModule& operator=(GPUExecutionModule&& rhs) noexcept = default;

			void AddRenderPassBundle(RenderPassBundle&& bundle);

			std::size_t GetRenderPassCount() const;
			Brawler::CompositeEnum<GPUCommandQueueType> GetUsedQueues() const;

			void PrepareForResourceStateTracking();
			
			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			std::vector<ResourceStateZone> GetResourceStateZones(const I_GPUResource& resource) const;

			bool IsResourceUsed(const I_GPUResource& resource) const;

			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			std::span<const std::unique_ptr<I_RenderPass<QueueType>>> GetRenderPassSpan() const;

			Brawler::SortedVector<I_GPUResource*> GetResourceDependencies() const;

			void SetModuleID(const std::uint32_t moduleID);

			void SubmitCommandListsForRenderPasses(const GPUExecutionModuleRecordContext& recordContext);

		private:
			template <GPUCommandQueueType QueueType>
			RenderPassContainer<QueueType>& GetRenderPassContainer();

			template <GPUCommandQueueType QueueType>
			const RenderPassContainer<QueueType>& GetRenderPassContainer() const;

			template <GPUCommandQueueType QueueType>
			void CreateCommandListRecorders();

			std::optional<std::unique_ptr<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>>> PrepareGPUResourceEvents(GPUResourceEventManager& eventManager);

		private:
			RenderPassContainer<GPUCommandQueueType::DIRECT> mDirectPassContainer;
			RenderPassContainer<GPUCommandQueueType::COMPUTE> mComputePassContainer;
			RenderPassContainer<GPUCommandQueueType::COPY> mCopyPassContainer;
			std::uint32_t mModuleID;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		const GPUExecutionModule::RenderPassContainer<QueueType>& GPUExecutionModule::GetRenderPassContainer() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectPassContainer;

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputePassContainer;

			else
				return mCopyPassContainer;
		}
		
		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		std::vector<ResourceStateZone> GPUExecutionModule::GetResourceStateZones(const I_GPUResource& resource) const
		{
			std::vector<ResourceStateZone> stateZoneArr{ GetRenderPassContainer<QueueType>().ResourceMap.GetResourceStateZones(resource) };

			for (auto& stateZone : stateZoneArr)
				stateZone.ExecutionModule = this;

			return stateZoneArr;
		}

		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		std::span<const std::unique_ptr<I_RenderPass<QueueType>>> GPUExecutionModule::GetRenderPassSpan() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::DIRECT>>>{ mDirectPassContainer.RenderPassArr };

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::COMPUTE>>>{ mComputePassContainer.RenderPassArr };

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::COPY>>>{ mCopyPassContainer.RenderPassArr };
		}
	}
}