module;
#include <vector>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.CommandAllocatorStorage;
import Brawler.D3D12.GPUCommandQueueType;

export namespace Brawler
{
	namespace D3D12
	{
		class CommandAllocatorStorage
		{
		private:
			struct CommandAllocatorInfo
			{
				Microsoft::WRL::ComPtr<Brawler::D3D12CommandAllocator> CmdAllocator;
				bool NeedsReset;
			};

		public:
			CommandAllocatorStorage() = default;

			CommandAllocatorStorage(const CommandAllocatorStorage& rhs) = delete;
			CommandAllocatorStorage& operator=(const CommandAllocatorStorage& rhs) = delete;

			CommandAllocatorStorage(CommandAllocatorStorage&& rhs) noexcept = default;
			CommandAllocatorStorage& operator=(CommandAllocatorStorage&& rhs) noexcept = default;

			void Initialize();

			/// <summary>
			/// Gets the ID3D12CommandAllocator instance for the calling thread. Since command allocators
			/// are *NOT* free-threaded, each thread in the system is assigned its own set of command
			/// allocators. Specifically, each thread is given Util::Engine::MAX_FRAMES_IN_FLIGHT
			/// ID3D12CommandAllocator instances, although each CommandAllocatorStorage instance only
			/// contains one command allocator per thread.
			/// 
			/// *NOTE*: This function *IS* thread safe.
			/// </summary>
			/// <returns>
			/// The function returns the ID3D12CommandAllocator instance for the calling thread.
			/// </returns>
			Brawler::D3D12CommandAllocator& GetD3D12CommandAllocator(const GPUCommandQueueType queueType);

			/// <summary>
			/// Calls ID3D12CommandAllocator::Reset() on each command allocator contained within this
			/// CommandAllocatorStorage. This function should only be called after a FrameGraph reset
			/// in order to ensure that we do not reset command allocators before the GPU can execute all
			/// of the commands written into them.
			/// </summary>
			void ResetCommandAllocators();

		private:
			template <GPUCommandQueueType QueueType>
			std::span<CommandAllocatorInfo> GetCommandAllocatorInfoSpan();

			template <GPUCommandQueueType QueueType>
			std::span<const CommandAllocatorInfo> GetCommandAllocatorInfoSpan() const;

		private:
			std::vector<CommandAllocatorInfo> mDirectCmdAllocatorArr;
			std::vector<CommandAllocatorInfo> mComputeCmdAllocatorArr;
			std::vector<CommandAllocatorInfo> mCopyCmdAllocatorArr;
		};
	}
}