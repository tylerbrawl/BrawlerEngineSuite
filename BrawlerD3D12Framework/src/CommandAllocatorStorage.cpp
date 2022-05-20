module;
#include <span>
#include <vector>
#include <thread>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.CommandAllocatorStorage;
import Util.General;
import Util.Engine;
import Util.Threading;
import Brawler.ThreadLocalResources;

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		std::span<CommandAllocatorStorage::CommandAllocatorInfo> CommandAllocatorStorage::GetCommandAllocatorInfoSpan()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectCmdAllocatorArr;

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeCmdAllocatorArr;

			else if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyCmdAllocatorArr;
		}

		template <GPUCommandQueueType QueueType>
		std::span<const CommandAllocatorStorage::CommandAllocatorInfo> CommandAllocatorStorage::GetCommandAllocatorInfoSpan() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectCmdAllocatorArr;

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeCmdAllocatorArr;

			else if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyCmdAllocatorArr;
		}

		void CommandAllocatorStorage::Initialize()
		{
			// Create a command allocator for each thread which might record commands. We
			// need to create a command allocator for each type of command list, unfortunately.
			static constexpr auto INITIALIZE_CMD_ALLOCATOR_ARRAY_LAMBDA = []<GPUCommandQueueType QueueType>(std::vector<CommandAllocatorInfo>& cmdAllocatorArr)
			{
				cmdAllocatorArr.resize(std::thread::hardware_concurrency());

				D3D12_COMMAND_LIST_TYPE allocatorType{};

				if constexpr (QueueType == GPUCommandQueueType::DIRECT)
					allocatorType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

				else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
					allocatorType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;

				else if constexpr (QueueType == GPUCommandQueueType::COPY)
					allocatorType = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;

				for (auto& cmdAllocatorInfo : cmdAllocatorArr)
					Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateCommandAllocator(allocatorType, IID_PPV_ARGS(&(cmdAllocatorInfo.CmdAllocator))));
			};

			INITIALIZE_CMD_ALLOCATOR_ARRAY_LAMBDA.operator() < GPUCommandQueueType::DIRECT > (mDirectCmdAllocatorArr);
			INITIALIZE_CMD_ALLOCATOR_ARRAY_LAMBDA.operator() < GPUCommandQueueType::COMPUTE > (mComputeCmdAllocatorArr);
			INITIALIZE_CMD_ALLOCATOR_ARRAY_LAMBDA.operator() < GPUCommandQueueType::COPY > (mCopyCmdAllocatorArr);
		}

		Brawler::D3D12CommandAllocator& CommandAllocatorStorage::GetD3D12CommandAllocator(const GPUCommandQueueType queueType)
		{
			const auto getCommandAllocatorLambda = [this]<GPUCommandQueueType QueueType>() -> Brawler::D3D12CommandAllocator&
			{
				const std::size_t currThreadIndex = static_cast<std::size_t>(Util::Threading::GetThreadLocalResources().GetThreadIndex());
				const std::span<CommandAllocatorInfo> cmdAllocatorInfoSpan{ GetCommandAllocatorInfoSpan<QueueType>() };

				assert(currThreadIndex < cmdAllocatorInfoSpan.size());
				assert(cmdAllocatorInfoSpan[currThreadIndex].CmdAllocator != nullptr);

				CommandAllocatorInfo& cmdAllocatorInfo{ cmdAllocatorInfoSpan[currThreadIndex] };

				cmdAllocatorInfo.NeedsReset = true;
				return *(cmdAllocatorInfo.CmdAllocator.Get());
			};
			
			switch (queueType)
			{
			case GPUCommandQueueType::DIRECT:
				return getCommandAllocatorLambda.operator() < GPUCommandQueueType::DIRECT > ();

			case GPUCommandQueueType::COMPUTE:
				return getCommandAllocatorLambda.operator() < GPUCommandQueueType::COMPUTE > ();

			case GPUCommandQueueType::COPY:
				return getCommandAllocatorLambda.operator() < GPUCommandQueueType::COPY > ();

			default:
			{
				assert(false && "ERROR: An invalid GPUCommandQueueType was provided to CommandAllocatorStorage::GetD3D12CommandAllocator()!");
				std::unreachable();

				return getCommandAllocatorLambda.operator() < GPUCommandQueueType::DIRECT > ();
			}
			}
		}

		void CommandAllocatorStorage::ResetCommandAllocators()
		{
			const auto resetAllocatorsLambda = [this]<GPUCommandQueueType QueueType>()
			{
				for (auto& cmdAllocatorInfo : GetCommandAllocatorInfoSpan<QueueType>())
				{
					if (cmdAllocatorInfo.NeedsReset)
					{
						Util::General::CheckHRESULT(cmdAllocatorInfo.CmdAllocator->Reset());
						cmdAllocatorInfo.NeedsReset = false;
					}
				}
			};
			
			resetAllocatorsLambda.operator() < GPUCommandQueueType::DIRECT > ();
			resetAllocatorsLambda.operator() < GPUCommandQueueType::COMPUTE > ();
			resetAllocatorsLambda.operator() < GPUCommandQueueType::COPY > ();
		}
	}
}