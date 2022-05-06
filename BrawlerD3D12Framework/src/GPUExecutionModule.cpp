module;
#include <vector>
#include <memory>
#include <ranges>
#include <cassert>
#include <optional>

module Brawler.D3D12.FrameGraphCompilation:GPUExecutionModule;
import Brawler.D3D12.RenderPassBundle;
import Util.Engine;
import Brawler.D3D12.GPUExecutionModuleRecordContext;
import Brawler.D3D12.AliasedGPUMemoryManager;
import Brawler.JobSystem;
import Brawler.D3D12.RenderPass;
import Brawler.D3D12.GPUCommandContextGroup;
import Brawler.D3D12.GPUCommandContextSubmissionPoint;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUCommandContextVault;

namespace
{
	/// <summary>
	/// This is the number of I_RenderPass instances which will be recorded into a single
	/// ID3D12GraphicsCommandList. This is different from the maximum number of
	/// I_RenderPass instances which can belong in a single GPUExecutionModule.
	/// 
	/// Specifically, each GPUExecutionModule represents one call to
	/// ExecuteCommandLists() (per queue). However, it would be inefficient to record
	/// all of the commands in a single GPUExecutionModule into one command list
	/// (per queue).
	/// 
	/// To allow for greater multithreading, each GPUExecutionModule can create multiple
	/// command lists (per queue) for recording.
	/// </summary>
	static constexpr std::size_t MAX_RENDER_PASSES_PER_COMMAND_LIST = 50;
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		GPUExecutionModule::RenderPassContainer<QueueType>& GPUExecutionModule::GetRenderPassContainer()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectPassContainer;

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputePassContainer;

			else
				return mCopyPassContainer;
		}
		
		template <GPUCommandQueueType QueueType>
		void GPUExecutionModule::CreateCommandListRecorders()
		{
			RenderPassContainer<QueueType>& passContainer{ GetRenderPassContainer<QueueType>() };

			const std::size_t totalRenderPassCount{ passContainer.RenderPassArr.size() };
			std::size_t renderPassesAdded = 0;

			while (renderPassesAdded < totalRenderPassCount)
			{
				const std::size_t passesToAddForCurrCmdList = std::min(MAX_RENDER_PASSES_PER_COMMAND_LIST, (totalRenderPassCount - renderPassesAdded));

				std::unique_ptr<GPUCommandListRecorder<QueueType>> cmdListRecorder{std::make_unique<GPUCommandListRecorder<QueueType>>()};
				cmdListRecorder->Reserve(passesToAddForCurrCmdList);

				for (auto&& renderPass : passContainer.RenderPassArr | std::views::drop(renderPassesAdded) | std::views::take(passesToAddForCurrCmdList))
					cmdListRecorder->AddRenderPass(std::move(renderPass));

				passContainer.CmdRecorderArr.push_back(std::move(cmdListRecorder));
				
				renderPassesAdded += passesToAddForCurrCmdList;
			}
		}

		GPUExecutionModule::GPUExecutionModule() :
			mDirectPassContainer(),
			mComputePassContainer(),
			mCopyPassContainer(),
			mModuleID(0)
		{}

		void GPUExecutionModule::AddRenderPassBundle(RenderPassBundle&& bundle)
		{
			for (auto&& directPass : bundle.GetRenderPassSpan<GPUCommandQueueType::DIRECT>())
				mDirectPassContainer.RenderPassArr.push_back(std::move(directPass));

			for (auto&& computePass : bundle.GetRenderPassSpan<GPUCommandQueueType::COMPUTE>())
				mComputePassContainer.RenderPassArr.push_back(std::move(computePass));

			for (auto&& copyPass : bundle.GetRenderPassSpan<GPUCommandQueueType::COPY>())
				mCopyPassContainer.RenderPassArr.push_back(std::move(copyPass));
		}

		std::size_t GPUExecutionModule::GetRenderPassCount() const
		{
			return (mDirectPassContainer.RenderPassArr.size() + mComputePassContainer.RenderPassArr.size() + mCopyPassContainer.RenderPassArr.size());
		}

		Brawler::CompositeEnum<GPUCommandQueueType> GPUExecutionModule::GetUsedQueues() const
		{
			Brawler::CompositeEnum<GPUCommandQueueType> usedQueues{};

			if (!mDirectPassContainer.RenderPassArr.empty())
				usedQueues |= GPUCommandQueueType::DIRECT;

			if (!mComputePassContainer.RenderPassArr.empty())
				usedQueues |= GPUCommandQueueType::COMPUTE;

			if (!mCopyPassContainer.RenderPassArr.empty())
				usedQueues |= GPUCommandQueueType::COPY;

			return usedQueues;
		}

		Brawler::SortedVector<I_GPUResource*> GPUExecutionModule::GetResourceDependencies() const
		{
			Brawler::SortedVector<I_GPUResource*> resourceDependencySet{};

			const auto addResourceDependenciesLambda = []<GPUCommandQueueType QueueType>(Brawler::SortedVector<I_GPUResource*>&dependencySet, const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassSpan)
			{
				for (const auto& renderPass : renderPassSpan)
				{
					for (const auto& dependency : renderPass->GetResourceDependencies())
						dependencySet.Insert(dependency.ResourcePtr);
				}
			};
			
			addResourceDependenciesLambda(resourceDependencySet, std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::DIRECT>>>{ mDirectPassContainer.RenderPassArr });
			addResourceDependenciesLambda(resourceDependencySet, std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::COMPUTE>>>{ mComputePassContainer.RenderPassArr });
			addResourceDependenciesLambda(resourceDependencySet, std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::COPY>>>{ mCopyPassContainer.RenderPassArr });

			return resourceDependencySet;
		}

		void GPUExecutionModule::SetModuleID(const std::uint32_t moduleID)
		{
			mModuleID = moduleID;
		}

		void GPUExecutionModule::SubmitCommandListsForRenderPasses(const GPUExecutionModuleRecordContext& recordContext)
		{
			// First, create all of the command list recorders by moving I_RenderPass
			// instances into them.
			CreateCommandListRecorders<GPUCommandQueueType::DIRECT>();
			CreateCommandListRecorders<GPUCommandQueueType::COMPUTE>();
			CreateCommandListRecorders<GPUCommandQueueType::COPY>();

			// Next, we need to make sure that all of the command list recorders can keep
			// track of aliasing barriers. We want to create a separate CPU job for each
			// command list to record within this GPUExecutionModule instance.
			//
			// However, these command lists are all submitted in the same call to
			// ExecuteCommandLists(). This means that they will need aliasing barriers
			// based on how the resources are used on the GPU timeline. This is not
			// really something which we can do on multiple threads.
			//
			// Therefore, what we do instead is track the resource usage on a single
			// thread, informing each command list recorder of which resources are
			// being used in which memory regions at the start of said recorder's
			// command list on the GPU timeline.
			
			AliasedGPUMemoryManager aliasedMemoryManager{ *(recordContext.AliasTracker) };

			const auto trackMemoryAliasingLambda = [this]<GPUCommandQueueType QueueType>(AliasedGPUMemoryManager& memoryManager)
			{
				RenderPassContainer<QueueType>& passContainer{ GetRenderPassContainer<QueueType>() };

				for (auto& cmdListRecorder : passContainer.CmdRecorderArr)
					cmdListRecorder->TrackResourceAliasing(memoryManager);
			};

			trackMemoryAliasingLambda.operator()<GPUCommandQueueType::DIRECT>(aliasedMemoryManager);
			trackMemoryAliasingLambda.operator()<GPUCommandQueueType::COMPUTE>(aliasedMemoryManager);
			trackMemoryAliasingLambda.operator()<GPUCommandQueueType::COPY>(aliasedMemoryManager);

			// Now, each command list recorder is aware of the aliasing barriers which it
			// will need to perform. However, we are not entirely finished yet.
			//
			// There is one problem which we must resolve ourselves. Specifically, the
			// GPUResourceEventManager may have GPUResourceEvents being sent to I_RenderPass
			// instances whose queue is not capable of handling said event.
			//
			// For instance, the D3D12 API requires that for a resource transition to be
			// valid on a given queue, that queue must support both the before and after
			// state of the transition. It isn't possible to know what the before state of the
			// resource is without first tracking its lifetime. To prevent race conditions,
			// however, we can't just add an I_RenderPass in the middle of resource analysis,
			// since that process is highly multithreaded.
			//
			// Instead, we assign impossible events to I_RenderPass instances with the
			// expectation that they will be resolved here. So, before we can record command
			// lists, we need to check if any of the GPUCommandListRecorders report that
			// one or more of their I_RenderPass instances is incapable of supporting all of
			// its transitions.
			//
			// In that case, we inject an I_RenderPass<GPUCommandQueueType::DIRECT> which will
			// execute in its own ExecuteCommandLists() call before the actual command lists
			// represented by the GPUCommandListRecorders found in the RenderPassContainers.
			//
			// This is a bit of a hacky solution, but at the time of writing this, I've already
			// spent about a month working on the FrameGraph system. Give me a break.
			std::optional<std::unique_ptr<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>>> unhandledEventRecorder{ PrepareGPUResourceEvents(*(recordContext.EventManager)) };

			const std::size_t numCmdListRecorders = (mDirectPassContainer.CmdRecorderArr.size() + mComputePassContainer.CmdRecorderArr.size() + mCopyPassContainer.CmdRecorderArr.size() 
				+ (unhandledEventRecorder.has_value() ? 1 : 0));
			assert(numCmdListRecorders > 0);

			Brawler::JobGroup cmdListRecordGroup{};
			cmdListRecordGroup.Reserve(numCmdListRecorders);

			if (unhandledEventRecorder.has_value()) [[unlikely]]
				cmdListRecordGroup.AddJob([recorderPtr = unhandledEventRecorder->get()]()
				{
					recorderPtr->RecordCommandList();
				});

			const auto addCmdListRecordJobLambda = [this]<GPUCommandQueueType QueueType>(Brawler::JobGroup& jobGroup)
			{
				RenderPassContainer<QueueType>& passContainer{ GetRenderPassContainer<QueueType>() };

				for (auto& cmdListRecorder : passContainer.CmdRecorderArr)
					jobGroup.AddJob([recorderPtr = cmdListRecorder.get()]() mutable
					{
						recorderPtr->RecordCommandList();
					});
			};

			addCmdListRecordJobLambda.operator()<GPUCommandQueueType::DIRECT>(cmdListRecordGroup);
			addCmdListRecordJobLambda.operator()<GPUCommandQueueType::COMPUTE>(cmdListRecordGroup);
			addCmdListRecordJobLambda.operator()<GPUCommandQueueType::COPY>(cmdListRecordGroup);

			cmdListRecordGroup.ExecuteJobs();

			// Once the command lists have been recorded, we can submit them to the GPU.
			// We create up to two GPUCommandContextGroups. If we needed to transfer any
			// GPUResourceEvents to a separate GPUCommandListRecorder, then this recorder's
			// GPUCommandContext gets placed in its own GPUCommandContextGroup.
			//
			// Regardless of whether or not we had to transfer any GPUResourceEvents, we
			// also create a GPUCommandContextGroup for all of the DIRECT, COMPUTE, and
			// COPY GPUCommandListRecorders we created from the original RenderPasses.

			std::vector<GPUCommandContextGroup> cmdContextGroupArr{};
			cmdContextGroupArr.reserve(1 + (unhandledEventRecorder.has_value() ? 1 : 0));

			if (unhandledEventRecorder.has_value()) [[unlikely]]
			{
				GPUCommandContextGroup unhandledEventGroup{};

				std::unique_ptr<DirectContext> eventHandlerContext{ (*unhandledEventRecorder)->ExtractGPUCommandContext() };
				unhandledEventGroup.AddGPUCommandContexts<GPUCommandQueueType::DIRECT>(std::span<std::unique_ptr<DirectContext>>{ &eventHandlerContext, 1 });

				cmdContextGroupArr.push_back(std::move(unhandledEventGroup));
			}

			const auto extractCmdContextLambda = [this]<GPUCommandQueueType QueueType>(GPUCommandContextGroup& cmdContextGroup)
			{
				using UniqueContextPtr = std::unique_ptr<GPUCommandQueueContextType<QueueType>>;

				RenderPassContainer<QueueType>& passContainer{ GetRenderPassContainer<QueueType>() };

				std::vector<UniqueContextPtr> extractedCmdContextArr{};
				extractedCmdContextArr.reserve(passContainer.CmdRecorderArr.size());

				for (auto&& cmdListRecorder : passContainer.CmdRecorderArr)
					extractedCmdContextArr.push_back(cmdListRecorder->ExtractGPUCommandContext());

				cmdContextGroup.AddGPUCommandContexts<QueueType>(std::span<UniqueContextPtr>{ extractedCmdContextArr });
			};

			GPUCommandContextGroup originalRecorderGroup{};

			extractCmdContextLambda.operator()<GPUCommandQueueType::DIRECT>(originalRecorderGroup);
			extractCmdContextLambda.operator()<GPUCommandQueueType::COMPUTE>(originalRecorderGroup);
			extractCmdContextLambda.operator()<GPUCommandQueueType::COPY>(originalRecorderGroup);

			cmdContextGroupArr.push_back(std::move(originalRecorderGroup));

			// Finally, submit the GPUCommandContextGroup instances created for this
			// GPUExecutionModule to its assigned GPUCommandContextSubmissionPoint. A
			// separate thread will then find them and submit them, in order, to the GPU.
			recordContext.SubmitPoint->SubmitGPUCommandContextGroups(std::move(cmdContextGroupArr));
		}

		std::optional<std::unique_ptr<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>>> GPUExecutionModule::PrepareGPUResourceEvents(GPUResourceEventManager& eventManager)
		{
			const auto prepareResourceEventsLambda = [this]<GPUCommandQueueType QueueType>(GPUResourceEventManager& eventManager, std::vector<GPUResourceEvent>& impossibleEventArr)
			{
				RenderPassContainer<QueueType>& passContainer{ GetRenderPassContainer<QueueType>() };

				for (auto& cmdListRecorder : passContainer.CmdRecorderArr)
				{
					std::vector<GPUResourceEvent> unhandledEventArr{ cmdListRecorder->PrepareGPUResourceEvents(eventManager) };

					impossibleEventArr.reserve(impossibleEventArr.size() + unhandledEventArr.size());

					for (auto&& unhandledEvent : unhandledEventArr)
						impossibleEventArr.push_back(std::move(unhandledEvent));
				}
			};

			std::vector<GPUResourceEvent> impossibleEventArr{};

			prepareResourceEventsLambda.operator()<GPUCommandQueueType::DIRECT>(eventManager, impossibleEventArr);
			prepareResourceEventsLambda.operator()<GPUCommandQueueType::COMPUTE>(eventManager, impossibleEventArr);
			prepareResourceEventsLambda.operator()<GPUCommandQueueType::COPY>(eventManager, impossibleEventArr);

			// If all of the GPUCommandListRecorder instances were able to handle all of their
			// GPUResourceEvents, then we don't need to create a separate command list for them.
			if (impossibleEventArr.empty()) [[likely]]
				return std::optional<std::unique_ptr<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>>>{};

			// Otherwise, we do need to create a command list dedicated to handling these
			// events.
			std::unique_ptr<I_RenderPass<GPUCommandQueueType::DIRECT>> eventHandlerPass{ std::make_unique<RenderPass<GPUCommandQueueType::DIRECT>>() };

			GPUResourceEventManager unhandledEventManager{};
			for (auto&& impossibleEvent : impossibleEventArr)
				unhandledEventManager.AddGPUResourceEvent(*eventHandlerPass, std::move(impossibleEvent));

			std::unique_ptr<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>> eventHandlerRecorder{ std::make_unique<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>>() };
			eventHandlerRecorder->AddRenderPass(std::move(eventHandlerPass));

			{
				std::vector<GPUResourceEvent> reallyImpossibleEventArr{ eventHandlerRecorder->PrepareGPUResourceEvents(unhandledEventManager) };
				assert(reallyImpossibleEventArr.empty());
			}
			
			return std::optional<std::unique_ptr<GPUCommandListRecorder<GPUCommandQueueType::DIRECT>>>{ std::move(eventHandlerRecorder) };
		}
	}
}