module;
#include <cassert>
#include <memory>
#include <format>
#include <assimp/scene.h>
#include <DirectXTex.h>

module Brawler.Application;
import Brawler.BC7CompressionRenderModule;
import Util.Win32;
import Brawler.JobGroup;
import Brawler.ModelTextureDatabase;

#pragma push_macro("AddJob")
#undef AddJob

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

	void Application::Run(LaunchParams&& launchParams)
	{
		mLaunchParams = std::move(launchParams);

		Util::Win32::WriteFormattedConsoleMessage(L"Beginning LOD mesh imports...");
		mModelResolver.Initialize();

		Util::Win32::WriteFormattedConsoleMessage(L"All LOD meshes have been imported. Initiating conversion sequence...");
		ExecuteModelConversionLoop();

		Util::Win32::WriteFormattedConsoleMessage(std::format(L"Conversion process completed. Exporting {}...", mLaunchParams.GetModelName()));
	}

	const LaunchParams& Application::GetLaunchParameters() const
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

	void Application::ExecuteModelConversionLoop()
	{
		// The model conversion loop is actually quite simple: Every "frame," we update the ModelResolver
		// and "render" the "frame." We use the term "frame" lightly here, since we aren't actually rendering
		// anything to the screen. However, the model exporter makes use of the FrameGraph system which
		// will be employed in the actual Brawler Engine in order to submit work to the GPU. This is done
		// for multiple reasons:
		//
		//   1. The FrameGraph system acts as a nice framework for submitting work to the GPU without having
		//      to make (many) native D3D12 API calls.
		//
		//   2. This serves as a nice stress test for the system before we actually put it into use in the
		//      Brawler Engine. The actual resource state management system is VERY complex, so we can use
		//      all of the testing which we can get.
		//
		// The model conversion loop executes until the ModelResolver reports that it is ready for serialization.

		while (!IsModelReadyForSerialization())
		{
			UpdateModelConversionComponents();
			ProcessFrame();
		}
	}

	void Application::UpdateModelConversionComponents()
	{
		Brawler::JobGroup updateComponentsGroup{};
		updateComponentsGroup.Reserve(2);

		updateComponentsGroup.AddJob([this] ()
		{
			mModelResolver.Update();
		});

		updateComponentsGroup.AddJob([] ()
		{
			ModelTextureDatabase::GetInstance().UpdateModelTextures();
		});

		updateComponentsGroup.ExecuteJobs();
	}

	void Application::ProcessFrame()
	{
		mRenderer.ProcessFrame();
		mRenderer.AdvanceFrame();
	}

	bool Application::IsModelReadyForSerialization() const
	{
		return (mModelResolver.IsReadyForSerialization() && ModelTextureDatabase::GetInstance().AreModelTexturesReadyForSerialization());
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

#pragma pop_macro("AddJob")