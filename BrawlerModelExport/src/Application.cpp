module;
#include <cassert>
#include <memory>
#include <assimp/scene.h>

module Brawler.Application;
import Brawler.AppParams;
import Brawler.BC7CompressionRenderModule;

namespace
{
	static Brawler::Application* appPtr = nullptr;
}

namespace Brawler
{
	Application::Application() :
		mThreadPool(),
		mRenderer(),
		mModelResolver(),
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

		// Add the I_RenderModules used by the application.
		mRenderer.AddRenderModule<BC7CompressionRenderModule>();
	}

	void Application::Run(AppParams&& appParams)
	{
		mLaunchParams = std::move(appParams);


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