module;
#include <memory>
#include <atomic>
#include <cassert>

export module Brawler.D3D12.AsyncGPUResourceBuilder;
import Brawler.D3D12.I_GPUResource;
import Brawler.JobSystem;
import Util.Engine;
import Util.Coroutine;
import Brawler.D3D12.PersistentGPUResourceManager;

namespace Brawler
{
	namespace D3D12
	{
		template <typename ResourceType, typename... Args>
		concept ConstructibleFrom = requires (Args&&... args)
		{
			ResourceType{ std::forward<Args>(args)... };
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename ResourceType>
			requires std::derived_from<ResourceType, I_GPUResource>
		class AsyncGPUResourceBuilder
		{
		public:
			template <typename... Args>
				requires ConstructibleFrom<ResourceType, Args...>
			AsyncGPUResourceBuilder(Args&&... args);
			
			template <typename... Args>
				requires ConstructibleFrom<ResourceType, Args...>
			AsyncGPUResourceBuilder(const Brawler::JobPriority jobPriority, Args&&... args);

			~AsyncGPUResourceBuilder();

			AsyncGPUResourceBuilder(const AsyncGPUResourceBuilder& rhs) = delete;
			AsyncGPUResourceBuilder& operator=(const AsyncGPUResourceBuilder& rhs) = delete;

			/// <summary>
			/// Waits for the derived I_GPUResource instance to be created, and then retrieves it.
			/// 
			/// If the resource was already created by the time this function is called, then the function
			/// returns immediately. Otherwise, it will call Util::Coroutine::TryExecuteJob() until it is
			/// finished.
			/// 
			/// Regardless of what happens, this function will never return unless and until the resource 
			/// is created.
			/// </summary>
			/// <returns>
			/// The function returns the derived I_GPUResource instance which was created by this
			/// AsyncGPUResourceBuilder instance.
			/// </returns>
			std::unique_ptr<ResourceType> GetCreatedGPUResource();

		private:
			template <typename... Args>
				requires ConstructibleFrom<ResourceType, Args...>
			void CreateGPUResourceCreationJob(const Brawler::JobPriority jobPriority, Args&&... args);

		private:
			std::unique_ptr<ResourceType> mResourcePtr;
			std::atomic<bool> mResourceCreatedFlag;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename ResourceType>
			requires std::derived_from<ResourceType, I_GPUResource>
		template <typename... Args>
			requires ConstructibleFrom<ResourceType, Args...>
		AsyncGPUResourceBuilder<ResourceType>::AsyncGPUResourceBuilder(Args&&... args) :
			mResourcePtr(nullptr),
			mResourceCreatedFlag(false)
		{
			CreateGPUResourceCreationJob(Brawler::JobPriority::LOW, std::forward<Args>(args)...);
		}

		template <typename ResourceType>
			requires std::derived_from<ResourceType, I_GPUResource>
		template <typename... Args>
			requires ConstructibleFrom<ResourceType, Args...>
		AsyncGPUResourceBuilder<ResourceType>::AsyncGPUResourceBuilder(const Brawler::JobPriority jobPriority, Args&&... args) :
			mResourcePtr(nullptr),
			mResourceCreatedFlag(false)
		{
			CreateGPUResourceCreationJob(jobPriority, std::forward<Args>(args)...);
		}

		template <typename ResourceType>
			requires std::derived_from<ResourceType, I_GPUResource>
		AsyncGPUResourceBuilder<ResourceType>::~AsyncGPUResourceBuilder()
		{
			assert(mResourceCreatedFlag.load(std::memory_order::relaxed) && "ERROR: An AsyncGPUResourceBuilder was destroyed before its resource could be created! (This is probably a *race condition!*)");
		}

		template <typename ResourceType>
			requires std::derived_from<ResourceType, I_GPUResource>
		std::unique_ptr<ResourceType> AsyncGPUResourceBuilder<ResourceType>::GetCreatedGPUResource()
		{
			while (!mResourceCreatedFlag.load(std::memory_order::acquire))
				Util::Coroutine::TryExecuteJob();

			return std::move(mResourcePtr);
		}

		template <typename ResourceType>
			requires std::derived_from<ResourceType, I_GPUResource>
		template <typename... Args>
			requires ConstructibleFrom<ResourceType, Args...>
		void AsyncGPUResourceBuilder<ResourceType>::CreateGPUResourceCreationJob(const Brawler::JobPriority jobPriority, Args&&... args)
		{
			// Since args is a parameter pack of references, we need to construct the derived I_GPUResource
			// instance *before* we leave this function. Don't worry about performance, though, as we don't
			// actually allocate any GPU memory in resource constructors.
			mResourcePtr = std::make_unique<ResourceType>(std::forward<Args>(args)...);
			
			Brawler::JobGroup creationJobGroup{ jobPriority, 1 };
			creationJobGroup.AddJob([this] ()
			{
				// Allocate the resource's memory on the GPU. Although this looks like just a simple function
				// call, allocating GPU memory can take a significant amount of time to complete. This is why
				// we wait until this job gets executed to do it.

				Util::Engine::GetPersistentGPUResourceManager().AllocatePersistentGPUResource(*mResourcePtr);

				// Write out the results for other threads.
				mResourceCreatedFlag.store(true, std::memory_order::release);
			});

			creationJobGroup.ExecuteJobsAsync();
		}
	}
}