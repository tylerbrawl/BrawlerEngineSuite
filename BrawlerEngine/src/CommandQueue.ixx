module;
#include "DxDef.h"

export module Brawler.CommandQueue;
import Brawler.CommandListType;

export namespace Brawler
{
	class CommandQueue
	{
	public:
		explicit CommandQueue(const Brawler::CommandListType cmdListType);

		Brawler::D3D12CommandQueue& GetD3D12CommandQueue();
		const Brawler::D3D12CommandQueue& GetD3D12CommandQueue() const;

	private:
		void Initialize(const Brawler::CommandListType cmdListType);

	private:
		Microsoft::WRL::ComPtr<Brawler::D3D12CommandQueue> mCmdQueue;
	};
}