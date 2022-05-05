module;
#include <cassert>
#include <set>
#include <span>
#include "DxDef.h"

module Brawler.RenderJobBundle;
import Brawler.I_RenderJob;
import Util.Engine;
import Brawler.I_GPUResource;
import Brawler.ResourceAccessMode;
import Brawler.RenderJobManager;

namespace
{
	bool CanResourceBeSimultaneouslyAccessed(const Brawler::I_GPUResource& resource)
	{
		const Brawler::D3D12_RESOURCE_DESC resourceDesc{ resource.GetResourceDescription() };

		// Only buffers or textures created with the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS
		// flag can be simultaneously accessed on multiple queues.
		return (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
	}
}

namespace Brawler
{
	RenderJobBundle::RenderJobBundle() :
		mGraphicsJobArr(),
		mComputeJobArr(),
		mCopyJobArr()
	{}

	void RenderJobBundle::AddGraphicsJob(GraphicsJob&& graphicsJob)
	{
		mGraphicsJobArr.push_back(std::move(graphicsJob));
		assert(CheckResourceDependencyConflicts() && "ERROR: An attempt was made to add an I_RenderJob with conflicting resource dependencies to a RenderJobBundle!");
	}

	std::span<GraphicsJob> RenderJobBundle::GetGraphicsJobs()
	{
		return mGraphicsJobArr;
	}

	std::size_t RenderJobBundle::GetGraphicsJobCount() const
	{
		return mGraphicsJobArr.size();
	}

	void RenderJobBundle::AddComputeJob(ComputeJob&& computeJob)
	{
		mComputeJobArr.push_back(std::move(computeJob));
		assert(CheckResourceDependencyConflicts() && "ERROR: An attempt was made to add an I_RenderJob with conflicting resource dependencies to a RenderJobBundle!");
	}

	std::span<ComputeJob> RenderJobBundle::GetComputeJobs()
	{
		return mComputeJobArr;
	}

	std::size_t RenderJobBundle::GetComputeJobCount() const
	{
		return mComputeJobArr.size();
	}

	void RenderJobBundle::AddCopyJob(CopyJob&& copyJob)
	{
		mCopyJobArr.push_back(std::move(copyJob));
		assert(CheckResourceDependencyConflicts() && "ERROR: An attempt was made to add an I_RenderJob with conflicting resource dependencies to a RenderJobBundle!");
	}

	std::span<CopyJob> RenderJobBundle::GetCopyJobs()
	{
		return mCopyJobArr;
	}

	std::size_t RenderJobBundle::GetCopyJobCount() const
	{
		return mCopyJobArr.size();
	}

	RenderEventHandle RenderJobBundle::SubmitRenderJobs()
	{
		return Util::Engine::GetRenderJobManager().SubmitRenderJobBundle(std::move(*this));
	}

	void RenderJobBundle::InitializeResourceTransitionManagers()
	{
		for (auto& graphicsJob : mGraphicsJobArr)
			graphicsJob.InitializeResourceTransitionManager();

		for (auto& computeJob : mComputeJobArr)
			computeJob.InitializeResourceTransitionManager();

		for (auto& copyJob : mCopyJobArr)
			copyJob.InitializeResourceTransitionManager();
	}

	bool RenderJobBundle::CheckResourceDependencyConflicts() const
	{
#ifdef _DEBUG
		/*
		There are restrictions imposed by both Direct3D 12 and the Brawler Engine as to which
		types of resources can be accessed simultaneously across multiple queues and/or
		command lists.

			- A resource cannot be accessed by more than one queue at a time, unless it is
			  either a buffer or created with the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS
			  flag.

			- If a resource dependency is created with Brawler::ResourceAccessMode::WRITE privileges,
			  then no other job in *ANY* queue within the same Brawler::RenderJobBundle may access
			  it.

			- If a resource dependency is created with Brawler::ResourceAccessMode::READ privileges,
			  then it may be accessed by other jobs with privileges no greater than
			  Brawler::ResourceAccessMode::READ, subject to the restrictions described above.
		
		This might not be the most efficient method to check for these restrictions, but since
		these checks are only done in Debug builds, this is an acceptable performance hit.
		*/

		std::set<I_GPUResource*> readAccessResources{};
		std::set<I_GPUResource*> writeAccessResources{};
		std::set<I_GPUResource*> graphicsJobResources{};
		std::set<I_GPUResource*> computeJobResources{};

		for (const auto& graphicsJob : mGraphicsJobArr)
		{
			for (const auto& itr : graphicsJob.mResourceDependencyMap)
			{
				if (writeAccessResources.contains(itr.first))
					return false;
				
				graphicsJobResources.insert(itr.first);

				if (itr.second == Brawler::ResourceAccessMode::WRITE)
				{
					if (!readAccessResources.contains(itr.first))
						writeAccessResources.insert(itr.first);
					else
						return false;
				}
				else
					readAccessResources.insert(itr.first);
			}
		}

		for (const auto& computeJob : mComputeJobArr)
		{
			for (const auto& itr : computeJob.mResourceDependencyMap)
			{
				if (writeAccessResources.contains(itr.first))
					return false;

				if (graphicsJobResources.contains(itr.first) && !CanResourceBeSimultaneouslyAccessed(*(itr.first)))
					return false;

				computeJobResources.insert(itr.first);

				if (itr.second == Brawler::ResourceAccessMode::WRITE)
				{
					if (!readAccessResources.contains(itr.first))
						writeAccessResources.insert(itr.first);
					else
						return false;
				}
				else
					readAccessResources.insert(itr.first);
			}
		}

		for (const auto& copyJob : mCopyJobArr)
		{
			for (const auto& itr : copyJob.mResourceDependencyMap)
			{
				if (writeAccessResources.contains(itr.first))
					return false;

				if ((graphicsJobResources.contains(itr.first) || computeJobResources.contains(itr.first)) && !CanResourceBeSimultaneouslyAccessed(*(itr.first)))
					return false;

				if (itr.second == Brawler::ResourceAccessMode::WRITE)
				{
					if (!readAccessResources.contains(itr.first))
						writeAccessResources.insert(itr.first);
					else
						return false;
				}
				else
					readAccessResources.insert(itr.first);
			}
		}
#endif // _DEBUG

		// In Release builds, we assume that everything is set up correctly. In fact, ideally,
		// this function won't even be called in Release builds. The idea is that we fix all
		// incorrect configurations in Debug builds so that we don't have to do these checks
		// in Release builds.
		return true;
	}
}