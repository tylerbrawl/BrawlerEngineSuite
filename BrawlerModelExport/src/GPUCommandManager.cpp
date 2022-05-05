module;
#include <functional>
#include <span>
#include <atomic>
#include "DxDef.h"

module Brawler.D3D12.GPUCommandManager;
import Util.Engine;
import Brawler.D3D12.GPUJobGroup;
import Brawler.JobSystem;
import Brawler.D3D12.DirectContext;
import Brawler.D3D12.ComputeContext;
import Brawler.D3D12.CopyContext;

namespace
{
	template <Brawler::D3D12::GPUCommandListType CmdListType>
	struct CommandContextMap
	{
		static_assert(sizeof(CmdListType) != sizeof(CmdListType));
	};

	template <typename T>
	struct CommandContextMapInstantiation
	{
		using CommandContextType = T;
	};

	template <>
	struct CommandContextMap<Brawler::D3D12::GPUCommandListType::DIRECT> : public CommandContextMapInstantiation<Brawler::D3D12::DirectContext>
	{};

	template <>
	struct CommandContextMap<Brawler::D3D12::GPUCommandListType::COMPUTE> : public CommandContextMapInstantiation<Brawler::D3D12::ComputeContext>
	{};

	template <>
	struct CommandContextMap<Brawler::D3D12::GPUCommandListType::COPY> : public CommandContextMapInstantiation<Brawler::D3D12::CopyContext>
	{};

	template <Brawler::D3D12::GPUCommandListType CmdListType>
	using CommandContextType = CommandContextMap<CmdListType>::CommandContextType;
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandListType CmdListType>
		struct GPUCommandManager::PrepareGPURecordJobsContext
		{
			Brawler::JobGroup& CmdListRecordGroup;
			std::vector<std::function<void(CommandContextType<CmdListType>&)>>&& ExtractedJobsArr;
			std::vector<CommandContextSubmitInfo>& ContextSubmitInfoArr;
		};

		void GPUCommandManager::ExchangeCurrentGPUEventHandle(const GPUEventHandle& hNextEventHandle)
		{
			std::shared_ptr<GPUEventHandle> hNextEventHandlePtr{ std::make_shared<GPUEventHandle>(hNextEventHandle) };
			
			// Get the current GPUEventHandle.
			std::shared_ptr<GPUEventHandle> hPrevEventHandle{ mCurrEventHandle.load() };
			
			// The thread remains in this loop until it is granted the right to submit
			// its command lists.
			while (true)
			{
				// Wait for the previous GPUEventHandle to be signalled. hPrevEventHandle
				// will only be nullptr the first time this function is called.
				if (hPrevEventHandle != nullptr) [[likely]]
					hPrevEventHandle->WaitForGPUExecution();

				// Try to swap the value of mCurrEventHandle with hNextEventHandle. If
				// we succeed, then that means that this thread has been granted the right
				// to submit its command lists to the GPU. If we fail, then another thread
				// has beat us to it, and we must wait for that thread's command lists to
				// finish execution.
				if (mCurrEventHandle.compare_exchange_strong(hPrevEventHandle, hNextEventHandlePtr))
					break;
			}
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandManager::PrepareGPURecordJobs(const PrepareGPURecordJobsContext<CmdListType>& context)
		{
			for (auto&& extractedJob : context.ExtractedJobsArr)
			{
				// By getting the command contexts on this thread and moving them into the CPU job, we
				// reduce contention on the GPUCommandContextVault.
				std::unique_ptr<CommandContextType<CmdListType>> cmdContext{ mCmdContextVault.AcquireCommandContext<CmdListType>() };

				// Increment the command context's required fence value. When we send it back to the command
				// context vault, the vault will see the updated fence value and refuse to allow the context
				// to be re-used until the GPU signals the fence with this value.
				cmdContext->IncrementRequiredFenceValue();

				// Extract the relevant information for submitting the command list to the GPU and notifying
				// CPU code of its completion.
				context.ContextSubmitInfoArr.push_back(CommandContextSubmitInfo{
					.CmdListPtr{ &(cmdContext->GetCommandList()) },
					.FencePtr{ &(cmdContext->GetFence()) },
					.NextFenceValue = cmdContext->GetRequiredFenceValue()
				});

				// On Windows, making DirectX 12 calls to record commands into a command list is a slow process,
				// particularly when compared to how long it takes on consoles. To alleviate this, we record our
				// commands into command lists in separate CPU jobs.
				//
				// This enables multiple threads to easily record into separate command lists. Each thread gets
				// its own command context, so we don't have to worry about multiple threads using the same
				// command list (or even the same command allocator).
				context.CmdListRecordGroup.AddJob([recordJob = std::move(extractedJob), cmdContextPtr = cmdContext.get()]()
				{
					cmdContextPtr->RecordCommandList(recordJob);
				});

				// Return the command context to the GPUCommandContextVault. Again, doing it like this can help
				// reduce contention on the vault.
				mCmdContextVault.ReturnCommandContext<CmdListType>(std::move(cmdContext));
			}
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandManager::SubmitCommandListsForExecution(const std::span<CommandContextSubmitInfo> contextSubmitSpan)
		{
			// Don't bother submitting anything if we don't have any command lists for this command queue.
			if (contextSubmitSpan.empty())
				return;
			
			std::vector<Brawler::D3D12GraphicsCommandList*> cmdListPtrArr{};
			cmdListPtrArr.reserve(contextSubmitSpan.size());

			for (const auto& submitInfo : contextSubmitSpan)
				cmdListPtrArr.push_back(submitInfo.CmdListPtr);

			// Submit the command lists to the relevant queue.
			Brawler::D3D12CommandQueue& cmdQueue{ GetCommandQueue<CmdListType>() };
			cmdQueue.ExecuteCommandLists(static_cast<std::uint32_t>(cmdListPtrArr.size()), reinterpret_cast<ID3D12CommandList* const*>(cmdListPtrArr.data()));

			for (const auto& submitInfo : contextSubmitSpan)
			{
				// Have the command queue signal each fence once all of the command lists have been executed.
				CheckHRESULT(cmdQueue.Signal(submitInfo.FencePtr, submitInfo.NextFenceValue));
			}
		}

		void GPUCommandManager::Initialize()
		{
			Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };

			// Create the direct command queue.
			D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{
				.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
				.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			};
			CheckHRESULT(d3dDevice.CreateCommandQueue(
				&cmdQueueDesc,
				IID_PPV_ARGS(&mDirectCmdQueue)
			));

			// Create the compute command queue.
			cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
			CheckHRESULT(d3dDevice.CreateCommandQueue(
				&cmdQueueDesc,
				IID_PPV_ARGS(&mComputeCmdQueue)
			));

			// Create the copy command queue.
			cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
			CheckHRESULT(d3dDevice.CreateCommandQueue(
				&cmdQueueDesc,
				IID_PPV_ARGS(&mCopyCmdQueue)
			));
		}

		GPUEventHandle GPUCommandManager::SubmitGPUJobGroup(GPUJobGroup&& jobGroup)
		{
			// Extract all of the recording jobs from the GPUJobGroup. We don't use a std::span because
			// the GPUJobGroup's contents won't be needed after this.
			std::vector<std::function<void(DirectContext&)>> directJobs{ jobGroup.ExtractDirectJobs() };
			std::vector<std::function<void(ComputeContext&)>> computeJobs{ jobGroup.ExtractComputeJobs() };
			std::vector<std::function<void(CopyContext&)>> copyJobs{ jobGroup.ExtractCopyJobs() };

			std::vector<CommandContextSubmitInfo> directSubmitInfoArr{};
			directSubmitInfoArr.reserve(directJobs.size());

			std::vector<CommandContextSubmitInfo> computeSubmitInfoArr{};
			computeSubmitInfoArr.reserve(computeJobs.size());

			std::vector<CommandContextSubmitInfo> copySubmitInfoArr{};
			copySubmitInfoArr.reserve(copyJobs.size());
			
			Brawler::JobGroup cmdListRecordGroup{};
			cmdListRecordGroup.Reserve(directJobs.size() + computeJobs.size() + copyJobs.size());

			// Prepare CPU jobs for recording all of the direct command lists.
			PrepareGPURecordJobs<GPUCommandListType::DIRECT>(PrepareGPURecordJobsContext<GPUCommandListType::DIRECT>{
				.CmdListRecordGroup{ cmdListRecordGroup },
				.ExtractedJobsArr{ std::move(directJobs) },
				.ContextSubmitInfoArr{ directSubmitInfoArr }
			});

			// Prepare CPU jobs for recording all of the compute command lists.
			PrepareGPURecordJobs<GPUCommandListType::COMPUTE>(PrepareGPURecordJobsContext<GPUCommandListType::COMPUTE>{
				.CmdListRecordGroup{ cmdListRecordGroup },
				.ExtractedJobsArr{ std::move(computeJobs) },
				.ContextSubmitInfoArr{ computeSubmitInfoArr }
			});

			// Prepare CPU jobs for recording all of the copy command lists.
			PrepareGPURecordJobs<GPUCommandListType::COPY>(PrepareGPURecordJobsContext<GPUCommandListType::COPY>{
				.CmdListRecordGroup{ cmdListRecordGroup },
				.ExtractedJobsArr{ std::move(copyJobs) },
				.ContextSubmitInfoArr{ copySubmitInfoArr }
			});

			// Execute the CPU jobs to record the command lists.
			cmdListRecordGroup.ExecuteJobs();

			// Prepare the GPUEventHandle which will notify us of the command lists
			// being executed on the GPU.
			GPUEventHandle hCompletionEvent{};
			{
				const auto PrepareGPUEventHandle = [] (Brawler::D3D12::GPUEventHandle& hCompletionEvent, const std::span<CommandContextSubmitInfo> contextSubmitSpan)
				{
					for (const auto& submitInfo : contextSubmitSpan)
					{
						// Add this fence to the GPUEventHandle which will be returned by GPUCommandManager::SubmitGPUJobBundle().
						//
						// Why do we do this instead of creating a new fence and having the command queue signal that?
						// There are two reasons for this:
						//
						//   1. Let's say we create a new ID3D12Fence and send it to hCompletionEvent. Then, this GPUEventHandle
						//      manages the lifetime of the ID3D12Fence. Now, let's say that whatever called
						//      GPUCommandManager::SubmitGPUJobBundle() does not need to check for GPU execution, and so it
						//      discards the returned GPUEventHandle. Then, when the command queue trys to signal this fence,
						//      it would be accessing garbage memory, since the ID3D12Fence would have been deallocated! On the
						//      other hand, we know that the ID3D12Fence instances owned by the command contexts will stay alive
						//      for as long as they do, and since we don't destroy these contexts, we can be sure that the
						//      command queue will be signalling a valid fence.
						//
						//   2. We need to signal the fences belonging to the command contexts anyways. Otherwise, the
						//      GPUCommandContextVault will never allow them to be re-used.
						hCompletionEvent.AddFence(*(submitInfo.FencePtr), submitInfo.NextFenceValue);
					}
				};

				PrepareGPUEventHandle(hCompletionEvent, directSubmitInfoArr);
				PrepareGPUEventHandle(hCompletionEvent, computeSubmitInfoArr);
				PrepareGPUEventHandle(hCompletionEvent, copySubmitInfoArr);
			}

			// Wait until this thread is granted the right to submit its command lists.
			ExchangeCurrentGPUEventHandle(hCompletionEvent);

			// Submit the command lists to the GPU for execution.
			SubmitCommandListsForExecution<GPUCommandListType::DIRECT>(directSubmitInfoArr);
			SubmitCommandListsForExecution<GPUCommandListType::COMPUTE>(computeSubmitInfoArr);
			SubmitCommandListsForExecution<GPUCommandListType::COPY>(copySubmitInfoArr);

			return hCompletionEvent;
		}
	}
}