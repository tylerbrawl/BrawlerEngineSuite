module;
#include "DxDef.h"

module Brawler.CommandQueue;
import Brawler.CommandListType;
import Util.Engine;

namespace Brawler
{
	CommandQueue::CommandQueue(const Brawler::CommandListType cmdListType) :
		mCmdQueue(nullptr)
	{
		Initialize(cmdListType);
	}

	Brawler::D3D12CommandQueue& CommandQueue::GetD3D12CommandQueue()
	{
		return *(mCmdQueue.Get());
	}

	const Brawler::D3D12CommandQueue& CommandQueue::GetD3D12CommandQueue() const
	{
		return *(mCmdQueue.Get());
	}

	void CommandQueue::Initialize(const Brawler::CommandListType cmdListType)
	{
		D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
		cmdQueueDesc.Type = Brawler::GetD3D12CommandListType(cmdListType);
		cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
		cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		
		CheckHRESULT(Util::Engine::GetD3D12Device().CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&mCmdQueue)));
	}
}