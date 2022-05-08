module;
#include <cassert>
#include <memory>
#include <assimp/scene.h>

module Brawler.Application;
import Brawler.AppParams;

import Brawler.Skeleton;

namespace
{
	static Brawler::Application* appPtr = nullptr;
}

namespace Brawler
{
	Application::Application() :
		mThreadPool(),
		mRenderer(),
		mSceneLoader(nullptr),
		mLaunchParams()
	{
		assert(appPtr == nullptr && "ERROR: An attempt was made to create a second instance of a Brawler::Application!");
		appPtr = this;

		Initialize();
	}

	void Application::Initialize()
	{
		mThreadPool.SetInitialized();

		mRenderer.Initialize();
	}

	void Application::Run(AppParams&& appParams)
	{
		mLaunchParams = std::move(appParams);
		mSceneLoader = std::make_unique<SceneLoader>(mLaunchParams.InputMeshFilePath);

		mSceneLoader->ProcessScene();
	}

	const SceneLoader& Application::GetSceneLoader() const
	{
		assert(mSceneLoader != nullptr);
		return *mSceneLoader;
	}

	const AppParams& Application::GetLaunchParameters() const
	{
		return mLaunchParams;
	}

	WorkerThreadPool& Application::GetWorkerThreadPool()
	{
		return mThreadPool;
	}

	const WorkerThreadPool& Application::GetWorkerThreadPool() const
	{
		return mThreadPool;
	}

	D3D12::Renderer& Application::GetRenderer()
	{
		return mRenderer;
	}

	const D3D12::Renderer& Application::GetRenderer() const
	{
		return mRenderer;
	}

	Application& GetApplication()
	{
		assert(appPtr != nullptr && "ERROR: An attempt was made to get the static Brawler::Application pointer before it could be initialized!");
		return *appPtr;
	}

	WorkerThreadPool& GetWorkerThreadPool()
	{
		thread_local WorkerThreadPool& threadPool{ GetApplication().GetWorkerThreadPool() };

		return threadPool;
	}

	D3D12::Renderer& GetRenderer()
	{
		thread_local D3D12::Renderer& renderer{ GetApplication().GetRenderer() };

		return renderer;
	}
}