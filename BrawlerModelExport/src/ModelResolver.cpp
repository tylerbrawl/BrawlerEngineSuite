module;
#include <memory>
#include <vector>

module Brawler.ModelResolver;
import Brawler.JobGroup;
import Brawler.AppParams;
import Util.ModelExport;

namespace Brawler
{
	void ModelResolver::InitializeLODResolvers()
	{
		const Brawler::AppParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::size_t lodCount = launchParams.GetLODCount();

		Brawler::JobGroup lodResolverCreationGroup{};
		lodResolverCreationGroup.Reserve(lodCount);

		mLODResolverPtrArr.resize(lodCount);

		std::size_t currLOD = 0;
		for (const auto& lodFilePath : launchParams.GetLODFilePaths())
		{
			std::unique_ptr<LODResolver>& lodResolverPtr{ mLODResolverPtrArr[currLOD] };
			lodResolverCreationGroup.AddJob([&lodResolverPtr, currLOD, &lodFilePath] ()
			{
				lodResolverPtr = std::make_unique<LODResolver>(static_cast<std::uint32_t>(currLOD));
				lodResolverPtr->ImportScene();
			});

			++currLOD;
		}

		lodResolverCreationGroup.ExecuteJobs();
	}

	void ModelResolver::Update()
	{
		Brawler::JobGroup updateGroup{};
		updateGroup.Reserve(mLODResolverPtrArr.size());

		for (const auto& lodResolver : mLODResolverPtrArr)
			updateGroup.AddJob([lodResolverPtr = lodResolver.get()](){ lodResolverPtr->Update(); });

		updateGroup.ExecuteJobs();
	}

	bool ModelResolver::IsReadyForSerialization() const
	{
		// This should not take long to check for each LODResolver instance, so we
		// will just check them all on a single thread.

		for (const auto& lodResolverPtr : mLODResolverPtrArr)
		{
			if (!lodResolverPtr->IsReadyForSerialization())
				return false;
		}

		return true;
	}
}