module;
#include <vector>
#include "DxDef.h"

export module Brawler.D3D12.GPUEventHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandManager;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUEventHandle
		{
		private:
			struct FenceInfo
			{
				Brawler::D3D12Fence* Fence;
				std::uint64_t RequiredFenceValue;
			};

		private:
			friend class GPUCommandManager;
			
		private:
			// Only the GPUCommandManager can create GPUEventHandle instances.
			GPUEventHandle() = default;

		public:
			~GPUEventHandle() = default;

			GPUEventHandle(const GPUEventHandle& rhs) = default;
			GPUEventHandle& operator=(const GPUEventHandle& rhs) = default;

			GPUEventHandle(GPUEventHandle&& rhs) noexcept = default;
			GPUEventHandle& operator=(GPUEventHandle&& rhs) noexcept = default;

			/// <summary>
			/// Checks if all of the GPU jobs related to this GPUEventHandle instance have
			/// been executed or not and returns the result.
			/// 
			/// NOTE: The return value of this function is based on the GPU's timeline; that
			/// is, it returns true when the GPU has finished executing the relevant
			/// commands. If this function returns false, then this does *NOT* mean that the
			/// commands have yet to be recorded; it only means that the GPU has not executed
			/// them yet.
			/// </summary>
			/// <returns>
			/// The function returns true if all of the GPU jobs related to this GPUEventHandle
			/// instance have been executed and false otherwise.
			/// </returns>
			bool HasGPUFinishedExecution() const;

			/// <summary>
			/// This is a utility function which calls Util::Coroutine::TryExecuteJob() until
			/// GPUEventHandle::HasGPUFinishedExecution() returns true for this instance.
			/// </summary>
			void WaitForGPUExecution() const;

		private:
			/// <summary>
			/// Assigns a new fence to this instance which must reach the value specified 
			/// by requiredFenceValue before GPUEventHandle::HasGPUFinishedExecution() returns
			/// true. This is intended for internal use only.
			/// </summary>
			/// <param name="fence">
			/// - The fence which is to be assigned to this GPUEventHandle instance.
			/// </param>
			/// <param name="requiredFenceValue">
			/// - The value which the specified fence must reach before
			///   GPUEventHandle::HasGPUFinishedExecution() can return true.
			/// </param>
			void AddFence(Brawler::D3D12Fence& fence, const std::uint64_t requiredFenceValue);

		private:
			std::vector<FenceInfo> mFenceInfoArr;
		};
	}
}