module;
#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <ranges>
#include <atomic>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.FrameGraph;
import Util.Coroutine;
import Util.Engine;
import Util.General;
import Brawler.JobSystem;
import Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.FrameGraphBuilder;
import Brawler.D3D12.GPUCommandContexts;
import Brawler.SortedVector;
import Brawler.D3D12.PersistentGPUResourceManager;
import Brawler.D3D12.GPUResidencyManager;
import Brawler.D3D12.GPUFence;
import Brawler.D3D12.GPUResourceDescriptorHeap;

namespace
{
	HRESULT AllocatePersistentGPUResources(const std::span<Brawler::D3D12::I_GPUResource* const> resourceDependencySpan)
	{
		using namespace Brawler::D3D12;

		// Persistent GPU resources, unlike transient GPU resources, may be either committed or placed.
		// Although there are a lot of locks in the placed resource creation path (which, unfortunately,
		// is likely to be the most common path), there are no shared locks in committed resource creation.
		//
		// What we can try to do, then, is create a job for each committed resource which needs to be
		// allocated, but allocate all placed resources on this thread. Of course, this assumes that the
		// driver doesn't use a lot of locks itself for committed allocations.

		std::vector<I_GPUResource*> committedResourceArr{};
		std::vector<I_GPUResource*> placedResourceArr{};

		for (const auto resourcePtr : resourceDependencySpan | std::views::filter([] (const I_GPUResource* const resourcePtr) { return (resourcePtr->GetGPUResourceLifetimeType() == GPUResourceLifetimeType::PERSISTENT && !resourcePtr->IsD3D12ResourceCreated()); }))
		{
			if (resourcePtr->GetPreferredCreationType() == GPUResourceCreationType::COMMITTED) [[unlikely]]
				committedResourceArr.push_back(resourcePtr);
			else
				placedResourceArr.push_back(resourcePtr);
		}

		std::atomic<std::size_t> committedResourceCreatedCount{ committedResourceArr.size() };
		
		// If we have any committed resources to create, then create them as separate CPU jobs.
		if (!committedResourceArr.empty()) [[unlikely]]
		{
			Brawler::JobGroup committedResourceCreationGroup{};
			committedResourceCreationGroup.Reserve(committedResourceArr.size());

			for (const auto committedResourcePtr : committedResourceArr)
				committedResourceCreationGroup.AddJob([committedResourcePtr, &committedResourceCreatedCount] ()
				{
					try
					{
						committedResourcePtr->CreateCommittedD3D12Resource();
						committedResourceCreatedCount.fetch_sub(1, std::memory_order::relaxed);
					}
					catch (...)
					{
						committedResourceCreatedCount.fetch_sub(1, std::memory_order::relaxed);
						std::rethrow_exception(std::current_exception());
					}
				});

			committedResourceCreationGroup.ExecuteJobsAsync();
		}

		// In the meantime, we will create the placed resources on this thread.
		HRESULT hr = S_OK;

		for (const auto placedResourcePtr : placedResourceArr)
		{
			hr = Util::Engine::GetPersistentGPUResourceManager().AllocatePersistentGPUResource(*placedResourcePtr);

			if (FAILED(hr)) [[unlikely]]
				break;
		}

		// Wait for the committed resources to be created.
		while (committedResourceCreatedCount.load(std::memory_order::relaxed) > 0)
			Util::Coroutine::TryExecuteJob();

		return hr;
	}

	HRESULT AllocateTransientGPUResources(Brawler::D3D12::TransientGPUResourceManager& transientResourceManager, const std::span<const std::vector<Brawler::D3D12::I_GPUResource*>> aliasableResourceGroupSpan)
	{
		// Each std::vector<I_GPUResource*> in aliasableResourceGroupSpan represents a set of I_GPUResource
		// instances which can alias each other.
		HRESULT hr = S_OK;

		for (const auto& aliasableResourceGroup : aliasableResourceGroupSpan)
		{
#ifdef _DEBUG
			assert(!aliasableResourceGroup.empty());

			const D3D12_HEAP_TYPE requiredHeapType = aliasableResourceGroup[0]->GetHeapType();

			for (const auto resourcePtr : aliasableResourceGroup | std::views::drop(1))
				assert(resourcePtr->GetHeapType() == requiredHeapType && "ERROR: An attempt was made to alias two transient I_GPUResource instances which were supposed to be placed into different types of heaps!");
#endif // _DEBUG

			hr = transientResourceManager.GetGPUResourceHeapManager().AllocateAliasedResources(std::span<Brawler::D3D12::I_GPUResource* const>{ aliasableResourceGroup }, aliasableResourceGroup[0]->GetHeapType());

			if (FAILED(hr)) [[unlikely]]
				return hr;
		}

		return hr;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void FrameGraph::Initialize()
		{
			mFenceCollection.Initialize();
			mTransientResourceManager.Initialize();
		}

		void FrameGraph::ProcessCurrentFrame(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan)
		{
			GenerateFrameGraph(renderModuleSpan);
			SubmitFrameGraph();
		}

		FrameGraphBlackboard& FrameGraph::GetBlackboard()
		{
			return mBlackboard;
		}

		const FrameGraphBlackboard& FrameGraph::GetBlackboard() const
		{
			return mBlackboard;
		}

		void FrameGraph::GenerateFrameGraph(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan)
		{
			ResetFrameGraph();

			std::vector<FrameGraphBuilder> frameGraphBuilderArr{ CreateFrameGraphBuilders(renderModuleSpan) };

			// Move all of the transient GPU resources into the TransientGPUResourceManager.
			for (auto& builder : frameGraphBuilderArr)
			{
				std::vector<std::unique_ptr<I_GPUResource>> builderTransientResourceArr{ builder.ExtractTransientResources() };
				mTransientResourceManager.AddTransientResources(builderTransientResourceArr);
			}

			// Compile the FrameGraph.
			mExecutionContext = CompileFrameGraph(std::span<FrameGraphBuilder>{ frameGraphBuilderArr });
		}

		void FrameGraph::SubmitFrameGraph()
		{
			mExecutionContext.SubmitFrameGraph(mFenceCollection);
		}

		void FrameGraph::WaitForPreviousFrameGraphExecution() const
		{
			mFenceCollection.WaitForFrameGraphCompletion();
		}

		void FrameGraph::ResetFrameGraph()
		{
			// Each FrameGraph instance is responsible for tracking the execution of a
			// given frame, as well as for managing the lifetime of transient resources
			// for that frame. We do not want to proceed with frame graph generation
			// until we can ensure that the last frame represented by this FrameGraph instance
			// has fully executed on the GPU.
			WaitForPreviousFrameGraphExecution();
			
			// All of the commands from the previous frame represented by this FrameGraph
			// instance have been executed on the GPU. It is now safe to perform various
			// clean-up actions as necessary.

			Util::Engine::GetGPUResourceDescriptorHeap().ResetPerFrameDescriptorHeapIndex();
			mBlackboard.ClearBlackboard();
			mTransientResourceManager.DeleteTransientResources();
			mFenceCollection.Reset();
		}

		std::vector<FrameGraphBuilder> FrameGraph::CreateFrameGraphBuilders(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan)
		{
			// Create a CPU job to get a FrameGraphBuilder from each enabled render
			// module.
			const auto isRenderModuleEnabledLambda = [] (const std::unique_ptr<I_RenderModule>& renderModule)
			{
				return renderModule->IsRenderModuleEnabled();
			};

			const std::size_t enabledRenderModulesCount = std::ranges::count_if(renderModuleSpan, isRenderModuleEnabledLambda);

			Brawler::JobGroup builderCreationGroup{};
			builderCreationGroup.Reserve(enabledRenderModulesCount);

			std::vector<FrameGraphBuilder> builderArr{};
			builderArr.reserve(enabledRenderModulesCount);

			for (std::size_t i = 0; i < enabledRenderModulesCount; ++i)
				builderArr.push_back(FrameGraphBuilder{ *this });

			std::size_t currBuilderIndex = 0;
			for (auto& renderModule : renderModuleSpan | std::views::filter(isRenderModuleEnabledLambda))
			{
				FrameGraphBuilder& currBuilder{ builderArr[currBuilderIndex++] };
				I_RenderModule* const currModulePtr = renderModule.get();

				builderCreationGroup.AddJob([&currBuilder, currModulePtr] ()
				{
					currBuilder = currModulePtr->CreateFrameGraphBuilder();
				});
			}

			builderCreationGroup.ExecuteJobs();

			// Assign IDs to each of the RenderPassBundles contained within all of the
			// FrameGraphBuilders.
			{
				std::uint32_t currBaseID = 0;

				for (auto& builder : builderArr)
				{
					builder.SetRenderPassBundleIDs(currBaseID);
					currBaseID += static_cast<std::uint32_t>(builder.GetRenderPassBundleCount());
				}
			}

			return builderArr;
		}

		FrameGraphExecutionContext FrameGraph::CompileFrameGraph(const std::span<FrameGraphBuilder> builderSpan)
		{
			// The first thing we want to figure out is which resources are going to be used,
			// as well as how long they are going to be used for. This allows us to decide
			// which resources we can alias in order to save memory.
			TransientGPUResourceAliasTracker aliasTracker{};
			aliasTracker.SetTransientGPUResourceHeapManager(mTransientResourceManager.GetGPUResourceHeapManager());

			for (auto& builder : builderSpan)
				builder.UpdateTransientGPUResourceAliasTracker(aliasTracker);

			aliasTracker.CalculateAliasableResources();

			// We now understand which resources we can/will alias together. With this information,
			// we can begin allocating GPU memory for our transient resources.
			//
			// As of writing this, GPU memory allocation is technically thread safe, but it is implemented
			// with a LOT of locks. As such, I believe that it would be best to leave a single thread to
			// do all of the allocations.
			//
			// However, we can still do these allocations concurrently with the compilation of the
			// FrameGraph.
			//
			// Before we do that, however, we will get all of the resource dependencies for the current
			// FrameGraph.
			Brawler::SortedVector<I_GPUResource*> resourceDependencySet{};
			for (auto& builder : builderSpan)
			{
				Brawler::SortedVector<I_GPUResource*> builderResourceDependencies{ builder.ExtractResourceDependencyCache() };
				builderResourceDependencies.ForEach([&resourceDependencySet] (I_GPUResource* const resourcePtr)
				{
					resourceDependencySet.Insert(resourcePtr);
				});
			}

			const std::span<const std::vector<I_GPUResource*>> aliasableResourceGroupSpan{ aliasTracker.GetAliasableResources() };
			std::atomic<bool> resourceAllocationFinished{ false };

			HRESULT hAllocationResult = S_OK;
			std::optional<GPUFence> gpuResidencyFence{};

			Brawler::JobGroup resourceCompilationGroup{};
			resourceCompilationGroup.Reserve(1);

			struct ResourceCompilationJobInfo
			{
				const std::span<I_GPUResource* const> ResourceDependencySpan;
				TransientGPUResourceManager& TransientResourceManager;
				const std::span<const std::vector<I_GPUResource*>> AliasableResourceGroupSpan;

				std::atomic<bool>& ResourceAllocationFinished;
				std::optional<GPUFence>& GPUResidencyFence;
				HRESULT& HAllocationResult;
			};
			ResourceCompilationJobInfo jobInfo{ resourceDependencySet.CreateSpan(), mTransientResourceManager, aliasableResourceGroupSpan, resourceAllocationFinished, gpuResidencyFence, hAllocationResult };

			resourceCompilationGroup.AddJob([this, &jobInfo]()
			{
				// Since there are no shared locks between persistent and transient GPU resource
				// memory allocation, we might see a performance gain by doing these concurrently.
				Brawler::JobGroup memoryAllocationGroup{};
				memoryAllocationGroup.Reserve(2);

				HRESULT hPersistentAllocationsResult = S_OK;
				memoryAllocationGroup.AddJob([&hPersistentAllocationsResult, &jobInfo] ()
				{
					hPersistentAllocationsResult = AllocatePersistentGPUResources(jobInfo.ResourceDependencySpan);
				});

				HRESULT hTransientAllocationsResult = S_OK;
				memoryAllocationGroup.AddJob([&hTransientAllocationsResult, &jobInfo] ()
				{
					hTransientAllocationsResult = AllocateTransientGPUResources(jobInfo.TransientResourceManager, jobInfo.AliasableResourceGroupSpan);
				});

				memoryAllocationGroup.ExecuteJobs();

				// Check for errors with the allocations.
				{
					HRESULT hTotalAllocationsResult = S_OK;

					if (FAILED(hPersistentAllocationsResult)) [[unlikely]]
						hTotalAllocationsResult = hPersistentAllocationsResult;

					else if (FAILED(hTransientAllocationsResult)) [[unlikely]]
						hTotalAllocationsResult = hTransientAllocationsResult;

					if (FAILED(hTotalAllocationsResult)) [[unlikely]]
					{
						jobInfo.HAllocationResult = hTotalAllocationsResult;
						jobInfo.ResourceAllocationFinished.store(true, std::memory_order::release);

						return;
					}
				}

				// Once we have allocated the memory for each GPU resource, we need to
				// check with the GPUResidencyManager.
				GPUResidencyManager::ResidencyPassResults residencyResults{ Util::Engine::GetGPUResidencyManager().ExecuteResidencyPass() };
				jobInfo.GPUResidencyFence = std::move(residencyResults.MakeResidentFence);
				jobInfo.HAllocationResult = residencyResults.HResult;

				jobInfo.ResourceAllocationFinished.store(true, std::memory_order::release);
			});

			// Begin creating the resources on the GPU. We do this asynchronously so as to not block
			// the current thread.
			resourceCompilationGroup.ExecuteJobsAsync();

			// In the meantime, we can go ahead and compile the FrameGraph, since doing so does not
			// require the GPU memory to actually be allocated.
			FrameGraphExecutionContext executionContext{};
			executionContext.CompileFrameGraph(builderSpan, std::move(aliasTracker));

			// Wait for the resources to be allocated on the GPU.
			while (!resourceAllocationFinished.load(std::memory_order::acquire))
				Util::Coroutine::TryExecuteJob();

			switch (hAllocationResult)
			{
			case S_OK:
				break;

			case E_OUTOFMEMORY:
			{
				throw std::runtime_error{ "ERROR: The GPU has ran out of memory!" };
				break;
			}
				
			default:
			{
				Util::General::CheckHRESULT(hAllocationResult);
				break;
			}
			}

			// If the GPUResidencyManager created a GPUFence, then that means that
			// we have evicted objects which need to be made resident before the GPU
			// can begin executing commands. In that case, we need to inform the
			// GPUCommandManager of this by adding the fence to the FrameGraphFenceCollection.
			if (gpuResidencyFence.has_value()) [[unlikely]]
				mFenceCollection.SetMakeResidentFence(std::move(*gpuResidencyFence));

			return executionContext;
		}
	}
}