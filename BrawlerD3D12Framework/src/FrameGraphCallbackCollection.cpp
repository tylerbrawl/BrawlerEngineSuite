module;
#include <cassert>
#include <functional>
#include <memory>

module Brawler.D3D12.FrameGraph;
import Brawler.JobSystem;

namespace Brawler
{
	namespace D3D12
	{
		void FrameGraphCallbackCollection::AddPersistentCallback(CallbackType&& callback)
		{
			// Invoking an empty std::move_only_function instance results in undefined behavior.
			assert(callback && "ERROR: An attempt was made to specify an empty std::move_only_function instance in a call to FrameGraphCallbackCollection::AddPersistentCallback()!");
			
			mPersistentCallbackArr.PushBack(std::make_unique<CallbackType>(std::move(callback)));
		}

		void FrameGraphCallbackCollection::AddTransientCallback(CallbackType&& callback)
		{
			// Invoking an empty std::move_only_function instance results in undefined behavior.
			assert(callback && "ERROR: An attempt was made to specify an empty std::move_only_function instance in a call to FrameGraphCallbackCollection::AddTransientCallback()!");
			
			mTransientCallbackArr.PushBack(std::move(callback));
		}

		void FrameGraphCallbackCollection::ExecuteFrameGraphCompletionCallbacks()
		{
			Brawler::JobGroup executionJobGroup{};

			// Capture each persistent CallbackType instance by reference, since we are going to
			// need it in subsequent frames.
			mPersistentCallbackArr.ForEach([&executionJobGroup] (const std::unique_ptr<CallbackType>& callbackPtr)
			{
				CallbackType& currCallback{ *callbackPtr };
				executionJobGroup.AddJob([&currCallback] () { currCallback(); });
			});

			// Move each transient CallbackType instance into the JobGroup. Rather than creating a CPU
			// job which executes the callback function, since CallbackType has the same function
			// signature as a CPU job, we can just pass the callback function into executionJobGroup
			// directly.
			mTransientCallbackArr.EraseIf([&executionJobGroup] (CallbackType& callback)
			{
				executionJobGroup.AddJob(std::move(callback));
				return true;
			});

			executionJobGroup.ExecuteJobs();
		}
	}
}