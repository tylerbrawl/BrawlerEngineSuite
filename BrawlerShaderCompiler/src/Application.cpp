module;
#include <cassert>
#include <fstream>
#include <span>

module Brawler.Application;
import Util.FileWrite;
import Brawler.DLLManager;

namespace
{
	static Brawler::Application* appPtr = nullptr;
}

namespace Brawler
{
	Application::Application(AppParams&& appParams) :
		mAppParams(std::move(appParams)),
		mThreadPool()
	{
		assert(appPtr == nullptr && "ERROR: An attempt was made to create a second instance of Brawler::Application!");
		appPtr = this;

		Initialize();
	}

	void Application::Initialize()
	{
		mThreadPool.SetInitialized();
	}

	void Application::Run()
	{
		Util::FileWrite::SerializeSourceFiles();
	}

	WorkerThreadPool& Application::GetWorkerThreadPool()
	{
		return mThreadPool;
	}

	const WorkerThreadPool& Application::GetWorkerThreadPool() const
	{
		return mThreadPool;
	}

	const AppParams& Application::GetLaunchParameters() const
	{
		return mAppParams;
	}

	Application& GetApplication()
	{
		assert(appPtr != nullptr && "ERROR: Brawler::GetApplication() was called before the static Brawler::Application pointer could be initialized!");
		return *appPtr;
	}
}