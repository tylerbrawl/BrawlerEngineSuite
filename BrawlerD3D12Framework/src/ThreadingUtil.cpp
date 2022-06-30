module;
#include <mutex>
#include <cstdint>
#include "DxDef.h"

module Util.Threading;
import Brawler.WorkerThreadPool;

namespace
{
	struct ThreadAffinityInfo
	{
		KAFFINITY CurrentMask;
		WORD CurrentProcessorGroup;

		ThreadAffinityInfo() :
			CurrentMask(1),
			CurrentProcessorGroup(0)
		{}
	};
}

namespace Brawler
{
	extern WorkerThreadPool& GetWorkerThreadPool();
}

namespace Util
{
	namespace Threading
	{
		void LockCurrentThreadToUnassignedCore()
		{
			static std::mutex currAffinityCritSection{};
			static ThreadAffinityInfo currAffinityInfo{};

			ThreadAffinityInfo unassignedInfo{};

			{
				std::scoped_lock<std::mutex> lk{ currAffinityCritSection };

				unassignedInfo = currAffinityInfo;

				currAffinityInfo.CurrentMask <<= 1;

				// If we have shifted left too many times, then there are no more free cores in the
				// current processor group.
				if (!currAffinityInfo.CurrentMask)
				{
					++currAffinityInfo.CurrentProcessorGroup;
					currAffinityInfo.CurrentMask = 1;
				}
			}

			GROUP_AFFINITY groupAffinity{};
			groupAffinity.Group = unassignedInfo.CurrentProcessorGroup;
			groupAffinity.Mask = unassignedInfo.CurrentMask;

			SetThreadGroupAffinity(GetCurrentThread(), &groupAffinity, nullptr);
		}

		bool IsMainThread()
		{
			Brawler::WorkerThreadPool& threadPool{ Brawler::GetWorkerThreadPool() };
			return (std::this_thread::get_id() == threadPool.GetMainThreadID());
		}

		Brawler::WorkerThread* GetCurrentWorkerThread()
		{
			if (IsMainThread())
				return nullptr;

			Brawler::WorkerThreadPool& threadPool = Brawler::GetWorkerThreadPool();
			return threadPool.GetWorkerThread(std::this_thread::get_id());
		}

		Brawler::ThreadLocalResources& GetThreadLocalResources()
		{
			// The main thread does not have a WorkerThread instance. Instead, its ThreadLocalResources
			// is stored inside of the WorkerThreadPool instance.

			if (Util::Threading::IsMainThread())
				return Brawler::GetWorkerThreadPool().mMainThreadInfo.Resources;

			return GetCurrentWorkerThread()->mResources;
		}

		std::size_t GetWorkerThreadCount()
		{
			return Brawler::GetWorkerThreadPool().GetWorkerThreadCount();
		}

		std::size_t GetTotalThreadCount()
		{
			return (GetWorkerThreadCount() + 1);
		}
	}
}