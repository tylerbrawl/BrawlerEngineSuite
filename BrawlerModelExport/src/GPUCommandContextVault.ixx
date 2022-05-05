module;
#include <memory>
#include <list>
#include <mutex>
#include <ranges>

export module Brawler.D3D12.GPUCommandContextVault;
import Brawler.D3D12.GPUCommandListType;
import Brawler.D3D12.DirectContext;
import Brawler.D3D12.ComputeContext;
import Brawler.D3D12.CopyContext;

namespace
{
	template <Brawler::D3D12::GPUCommandListType CmdListType>
	struct ContextLockerInfo
	{
		static_assert(sizeof(CmdListType) != sizeof(CmdListType), "ERROR: An explicit specialization of ContextLockerInfo was never provided for a particular GPUCommandListType! (See GPUCommandContextVault.ixx.)");
	};

	template <typename ContextType_>
	struct ContextLockerInfoInstantiation
	{
		using ContextType = ContextType_;
	};

	template <>
	struct ContextLockerInfo<Brawler::D3D12::GPUCommandListType::DIRECT> : public ContextLockerInfoInstantiation<Brawler::D3D12::DirectContext>
	{};

	template <>
	struct ContextLockerInfo<Brawler::D3D12::GPUCommandListType::COMPUTE> : public ContextLockerInfoInstantiation<Brawler::D3D12::ComputeContext>
	{};

	template <>
	struct ContextLockerInfo<Brawler::D3D12::GPUCommandListType::COPY> : public ContextLockerInfoInstantiation<Brawler::D3D12::CopyContext>
	{};

	template <Brawler::D3D12::GPUCommandListType CmdListType>
	using CommandListContextType = ContextLockerInfo<CmdListType>::ContextType;
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextVault
		{
		private:
			template <typename T>
			struct ContextLocker
			{
				std::vector<std::unique_ptr<T>> ContextList;
				std::mutex CritSection;
			};

		public:
			GPUCommandContextVault() = default;

			GPUCommandContextVault(const GPUCommandContextVault& rhs) = delete;
			GPUCommandContextVault& operator=(const GPUCommandContextVault& rhs) = delete;

			GPUCommandContextVault(GPUCommandContextVault&& rhs) noexcept = default;
			GPUCommandContextVault& operator=(GPUCommandContextVault&& rhs) noexcept = default;

			/// <summary>
			/// If possible, this function will return a previous command context whose commands
			/// have already been executed by the GPU. Otherwise, this function creates a new
			/// command context.
			/// 
			/// Regardless of what happens, the returned command context can be immediately used
			/// for recording GPU commands.
			/// </summary>
			/// <returns>
			/// The function returns a command context which can be immediately used for recording
			/// GPU commands.
			/// </returns>
			template <GPUCommandListType CmdListType>
			std::unique_ptr<CommandListContextType<CmdListType>> AcquireCommandContext();

			/// <summary>
			/// Stores the provided command context back into this GPUCommandContextVault. 
			/// 
			/// It is safe to do this even before the GPU has finished executing its commands; the
			/// GPUCommandContextVault will never return a command context whose commands have
			/// not been executed when GPUCommandContextVault::AcquireCommandContext() is called.
			/// 
			/// Returning command contexts is important to ensure that we only create additional
			/// command allocators as needed.
			/// </summary>
			/// <param name="cmdContext">
			/// - The command context which is to be returned to this GPUCommandContextVault.
			/// </param>
			template <GPUCommandListType CmdListType>
			void ReturnCommandContext(std::unique_ptr<CommandListContextType<CmdListType>>&& cmdContext);

		private:
			template <GPUCommandListType CmdListType>
			ContextLocker<CommandListContextType<CmdListType>>& GetContextLocker();

			template <GPUCommandListType CmdListType>
			const ContextLocker<CommandListContextType<CmdListType>>& GetContextLocker() const;

		private:
			ContextLocker<DirectContext> mDirectContextLocker;
			ContextLocker<ComputeContext> mComputeContextLocker;
			ContextLocker<CopyContext> mCopyContextLocker;
		};
	}
}

// -------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandListType CmdListType>
		std::unique_ptr<CommandListContextType<CmdListType>> GPUCommandContextVault::AcquireCommandContext()
		{
			using ContextPtr_T = std::unique_ptr<CommandListContextType<CmdListType>>;
			
			ContextPtr_T contextPtr{ nullptr };
			ContextLocker<CommandListContextType<CmdListType>>& contextLocker{ GetContextLocker<CmdListType>() };

			// Try to acquire a command context from the ContextLocker. We only consider those whose
			// commands were already executed by the GPU.
			{
				std::scoped_lock<std::mutex> lock{ contextLocker.CritSection };

				auto itr = std::ranges::find_if(contextLocker.ContextList, [] (const ContextPtr_T& context) { return context->ReadyForUse(); });
				if (itr != contextLocker.ContextList.end())
				{
					// We found a context which we can use.
					contextPtr = std::move(*itr);
					contextLocker.ContextList.erase(itr);
				}
			}

			// If we do not have a free command context, then we need to create one.
			if (contextPtr == nullptr)
				contextPtr = std::make_unique<CommandListContextType<CmdListType>>();

			return contextPtr;
		}

		template <GPUCommandListType CmdListType>
		void GPUCommandContextVault::ReturnCommandContext(std::unique_ptr<CommandListContextType<CmdListType>>&& cmdContext)
		{
			ContextLocker<CommandListContextType<CmdListType>>& contextLocker{ GetContextLocker<CmdListType>() };

			{
				std::scoped_lock<std::mutex> lock{ contextLocker.CritSection };
				contextLocker.ContextList.push_back(std::move(cmdContext));
			}
		}

		template <GPUCommandListType CmdListType>
		GPUCommandContextVault::ContextLocker<CommandListContextType<CmdListType>>& GPUCommandContextVault::GetContextLocker()
		{
			if constexpr (CmdListType == GPUCommandListType::DIRECT)
				return mDirectContextLocker;

			if constexpr (CmdListType == GPUCommandListType::COMPUTE)
				return mComputeContextLocker;

			if constexpr (CmdListType == GPUCommandListType::COPY)
				return mCopyContextLocker;
		}

		template <GPUCommandListType CmdListType>
		const GPUCommandContextVault::ContextLocker<CommandListContextType<CmdListType>>& GPUCommandContextVault::GetContextLocker() const
		{
			if constexpr (CmdListType == GPUCommandListType::DIRECT)
				return mDirectContextLocker;

			if constexpr (CmdListType == GPUCommandListType::COMPUTE)
				return mComputeContextLocker;

			if constexpr (CmdListType == GPUCommandListType::COPY)
				return mCopyContextLocker;
		}
	}
}