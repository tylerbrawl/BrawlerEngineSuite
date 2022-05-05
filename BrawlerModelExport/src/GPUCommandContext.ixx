module;
#include <atomic>
#include <cassert>
#include <functional>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUCommandListType;
import Util.Engine;
import Brawler.D3D12.GPUBarrierGroup;
import Brawler.D3D12.GPUResourceHandle;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class DirectContext;
		class ComputeContext;
		class CopyContext;
	}
}

namespace
{
	template <Brawler::D3D12::GPUCommandListType CmdListType>
	struct GPUCommandContextInfo
	{
		static_assert(sizeof(CmdListType) != sizeof(CmdListType), "ERROR: An explicit template specialization was not provided for GPUCommandContextInfo with a given Brawler::GPUCommandListType! (See GPUCommandContext.ixx.)");
	};

	template <D3D12_COMMAND_LIST_TYPE D3DCmdListType, typename ContextType_>
	struct GPUCommandContextInfoInstantiation
	{
		static constexpr D3D12_COMMAND_LIST_TYPE D3D_CMD_LIST_TYPE = D3DCmdListType;
		
		using ContextType = ContextType_;
	};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandListType::DIRECT> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, Brawler::D3D12::DirectContext>
	{};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandListType::COMPUTE> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE, Brawler::D3D12::ComputeContext>
	{};

	template <>
	struct GPUCommandContextInfo<Brawler::D3D12::GPUCommandListType::COPY> : public GPUCommandContextInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, Brawler::D3D12::CopyContext>
	{};
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandManager;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextVault;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandListType CmdListType>
		class GPUCommandContext
		{
		private:
			friend class GPUCommandManager;
			friend class GPUCommandContextVault;

		private:
			using DerivedContextType = GPUCommandContextInfo<CmdListType>::ContextType;

		protected:
			GPUCommandContext();

		public:
			virtual ~GPUCommandContext();

			GPUCommandContext(const GPUCommandContext& rhs) = delete;
			GPUCommandContext& operator=(const GPUCommandContext& rhs) = delete;

			GPUCommandContext(GPUCommandContext&& rhs) noexcept = default;
			GPUCommandContext& operator=(GPUCommandContext&& rhs) noexcept = default;

		protected:
			Brawler::D3D12GraphicsCommandList& GetCommandList();
			const Brawler::D3D12GraphicsCommandList& GetCommandList() const;

		public:
			void ResourceBarrier(GPUBarrierGroup&& barrierGroup);

			void CopyResource(const GPUResourceWriteHandle hDestination, const GPUResourceReadHandle hSource);

		private:
			Brawler::D3D12Fence& GetFence();
			const Brawler::D3D12Fence& GetFence() const;

			void RecordCommandList(const std::function<void(DerivedContextType&)>& recordJob);
			virtual void RecordCommandListIMPL(const std::function<void(DerivedContextType&)>& recordJob) = 0;

			/// <summary>
			/// Resets both the command list *AND* its underlying allocator. The latter action
			/// means that this function can only be called *AFTER* the GPU has finished
			/// executing all of the commands of this GPUCommandContext instance.
			/// </summary>
			void ResetCommandList();

			/// <summary>
			/// Checks if this GPUCommandContext can be used to record other commands. This
			/// function will return true only if the GPU has finished executing all of the
			/// previous commands.
			/// </summary>
			/// <returns>
			/// The function returns true if the GPU has finished executing all of the commands
			/// of this GPUCommandContext and false otherwise.
			/// </returns>
			bool ReadyForUse() const;

			void IncrementRequiredFenceValue();
			std::uint64_t GetRequiredFenceValue() const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12CommandAllocator> mCmdAllocator;
			Microsoft::WRL::ComPtr<Brawler::D3D12GraphicsCommandList> mCmdList;
			Microsoft::WRL::ComPtr<Brawler::D3D12Fence> mFence;

			/// <summary>
			/// This is the value which the result of mFence->GetCompletedValue() must match
			/// before this context can be used again. It is atomic in order to trigger a
			/// CPU cache flush when it is updated.
			/// </summary>
			std::atomic<std::uint64_t> mRequiredFenceValue;
		};
	}
}

// ---------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandListType CmdListType>
		GPUCommandContext<CmdListType>::GPUCommandContext() :
			mCmdAllocator(nullptr),
			mCmdList(nullptr),
			mFence(nullptr),
			mRequiredFenceValue(0)
		{
			Brawler::D3D12Device& d3dDevice{ Util::Engine::GetD3D12Device() };

			{
				// Create the fence for synchronization.
				Microsoft::WRL::ComPtr<ID3D12Fence> fence{};
				CheckHRESULT(d3dDevice.CreateFence(
					0,
					D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
					IID_PPV_ARGS(&mFence)
				));

				CheckHRESULT(fence.As(&mFence));
			}

			{
				// Create the command allocator.
				CheckHRESULT(d3dDevice.CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(&mCmdAllocator)
				));
			}

			{
				// Create the command list in the closed state.
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList{};
				CheckHRESULT(d3dDevice.CreateCommandList1(
					0,
					GPUCommandContextInfo<CmdListType>::D3D_CMD_LIST_TYPE,
					D3D12_COMMAND_LIST_FLAGS::D3D12_COMMAND_LIST_FLAG_NONE,
					IID_PPV_ARGS(&cmdList)
				));

				CheckHRESULT(cmdList.As(&mCmdList));
			}
		}

		template <GPUCommandListType CmdListType>
		GPUCommandContext<CmdListType>::~GPUCommandContext()
		{
			assert(ReadyForUse() && "ERROR: A command context was destroyed before the GPU could execute all of its commands!");
		}

		template <GPUCommandListType CmdListType>
		Brawler::D3D12GraphicsCommandList& GPUCommandContext<CmdListType>::GetCommandList()
		{
			return *(mCmdList.Get());
		}

		template <GPUCommandListType CmdListType>
		const Brawler::D3D12GraphicsCommandList& GPUCommandContext<CmdListType>::GetCommandList() const
		{
			return *(mCmdList.Get());
		}

		template <GPUCommandListType CmdListType>
		Brawler::D3D12Fence& GPUCommandContext<CmdListType>::GetFence()
		{
			return *(mFence.Get());
		}

		template <GPUCommandListType CmdListType>
		const Brawler::D3D12Fence& GPUCommandContext<CmdListType>::GetFence() const
		{
			return *(mFence.Get());
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandContext<CmdListType>::ResourceBarrier(GPUBarrierGroup&& barrierGroup)
		{
			const std::span<const CD3DX12_RESOURCE_BARRIER> barrierSpan{ barrierGroup.GetResourceBarriers() };
			mCmdList->ResourceBarrier(static_cast<std::uint32_t>(barrierSpan.size()), barrierSpan.data());

			// In Debug builds, we reset the GPUBarrierGroup to make sure that we don't use it again.
#ifdef _DEBUG
			barrierGroup = GPUBarrierGroup{};
#endif // _DEBUG
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandContext<CmdListType>::CopyResource(const GPUResourceWriteHandle hDestination, const GPUResourceReadHandle hSource)
		{
			GetCommandList().CopyResource(&(hDestination->GetD3D12Resource()), &(hSource->GetD3D12Resource()));
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandContext<CmdListType>::RecordCommandList(const std::function<void(DerivedContextType&)>& recordJob)
		{
			// Reset the command list so that we can record commands into it.
			ResetCommandList();

			// Record the commands into the command list.
			RecordCommandListIMPL(recordJob);

			// Close the command list. This lets D3D12 know that we are finished recording commands
			// into it.
			CheckHRESULT(mCmdList->Close());
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandContext<CmdListType>::ResetCommandList()
		{
			assert(ReadyForUse() && "ERROR: An attempt was made to record commands into a command context, but the GPU was not finished executing all of its previous commands!");

			CheckHRESULT(mCmdAllocator->Reset());
			CheckHRESULT(mCmdList->Reset(mCmdAllocator.Get(), nullptr));
		}

		template <GPUCommandListType CmdListType>
		bool GPUCommandContext<CmdListType>::ReadyForUse() const
		{
			const std::uint64_t requiredValue = mRequiredFenceValue.load();

			// The required fence value should never be less than the completed fence
			// value; it should only be >= it.
			assert(requiredValue >= mFence->GetCompletedValue() && "ERROR: A command context was used again before all of its commands could be executed by the GPU!");

			return requiredValue == mFence->GetCompletedValue();
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandContext<CmdListType>::IncrementRequiredFenceValue()
		{
			++mRequiredFenceValue;
		}

		template <GPUCommandListType CmdListType>
		std::uint64_t GPUCommandContext<CmdListType>::GetRequiredFenceValue() const
		{
			return mRequiredFenceValue.load();
		}
	}
}