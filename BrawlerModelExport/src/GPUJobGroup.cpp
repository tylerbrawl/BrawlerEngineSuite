module;
#include <functional>
#include <vector>
#include <cassert>
#include <unordered_map>

module Brawler.D3D12.GPUJobGroup;
import Util.Engine;
import Brawler.D3D12.GPUCommandManager;

namespace Brawler
{
	namespace D3D12
	{
		void GPUJobGroup::AddDirectJob(std::function<void(DirectContext&)>&& job)
		{
			mDirectJobArr.push_back(std::move(job));
		}

		void GPUJobGroup::AddComputeJob(std::function<void(ComputeContext&)>&& job)
		{
			mComputeJobArr.push_back(std::move(job));
		}

		void GPUJobGroup::AddCopyJob(std::function<void(CopyContext&)>&& job)
		{
			mCopyJobArr.push_back(std::move(job));
		}

		GPUEventHandle GPUJobGroup::ExecuteJobs()
		{
			return Util::Engine::GetGPUCommandManager().SubmitGPUJobGroup(std::move(*this));
		}

		GPUResourceReadHandle GPUJobGroup::CreateGPUResourceReadHandle(I_GPUResource& resource)
		{
#ifdef _DEBUG
			assert(CanGPUResourceHandleBeCreated(resource, GPUResourceAccessMode::READ_ONLY) && "ERROR: A request to create a GPUResourceReadHandle for an I_GPUResource was denied!");
			RegisterCreatedGPUResourceHandle(resource, GPUResourceAccessMode::READ_ONLY);
#endif // _DEBUG

			return GPUResourceReadHandle{ resource };
		}

		GPUResourceWriteHandle GPUJobGroup::CreateGPUResourceWriteHandle(I_GPUResource& resource)
		{
#ifdef _DEBUG
			assert(CanGPUResourceHandleBeCreated(resource, GPUResourceAccessMode::READ_WRITE) && "ERROR: A request to create a GPUResourceReadHandle for an I_GPUResource was denied!");
			RegisterCreatedGPUResourceHandle(resource, GPUResourceAccessMode::READ_WRITE);
#endif // _DEBUG

			return GPUResourceWriteHandle{ resource };
		}

		bool GPUJobGroup::CanGPUResourceHandleBeCreated(const I_GPUResource& resource, const GPUResourceAccessMode desiredAccessMode) const
		{
#ifdef _DEBUG
			if (!mAllowedResourceHandlesMap.contains(&resource))
				return true;

			// We allow either one of the following:
			//
			//   1. One or more GPUResourceReadHandles.
			//   2. One (and only one) GPUResourceWriteHandle.

			// If we have already created a read handle, then allow only other read handles.
			if (mAllowedResourceHandlesMap.at(&resource) == GPUResourceAccessMode::READ_ONLY)
				return (desiredAccessMode == GPUResourceAccessMode::READ_ONLY);

			// If we have already created a write handle, then do not allow any other handles for
			// this I_GPUResource instance.
			return false;
#else
			return true;
#endif // _DEBUG
		}

		void GPUJobGroup::RegisterCreatedGPUResourceHandle(const I_GPUResource& resource, const GPUResourceAccessMode desiredAccessMode)
		{
#ifdef _DEBUG
			mAllowedResourceHandlesMap[&resource] = desiredAccessMode;
#endif // _DEBUG
		}

		std::vector<std::function<void(DirectContext&)>> GPUJobGroup::ExtractDirectJobs()
		{
			return std::move(mDirectJobArr);
		}

		std::vector<std::function<void(ComputeContext&)>> GPUJobGroup::ExtractComputeJobs()
		{
			return std::move(mComputeJobArr);
		}

		std::vector<std::function<void(CopyContext&)>> GPUJobGroup::ExtractCopyJobs()
		{
			return std::move(mCopyJobArr);
		}
	}
}