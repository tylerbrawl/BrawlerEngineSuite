module;
#include <cassert>
#include "DxDef.h"

module Brawler.Application;
import Brawler.SettingsManager;
import Brawler.SettingID;
import Brawler.GPUSceneUpdateRenderModule;

namespace
{
	static Brawler::Application* appInstance = nullptr;

	static constexpr float PHYSICS_TIMESTEP_S = (1.0f / 60.0f);
	static constexpr float PHYSICS_TIMESTEP_MS = PHYSICS_TIMESTEP_S * 1000.0f;
	static const std::uint64_t PHYSICS_TIMESTEP_TICKS = [] ()
	{
		LARGE_INTEGER li{};
		QueryPerformanceFrequency(&li);

		return static_cast<std::uint64_t>(PHYSICS_TIMESTEP_S * li.QuadPart);
	}();

	__forceinline std::uint64_t GetCurrentTickCount()
	{
		LARGE_INTEGER li{};
		QueryPerformanceCounter(&li);

		return li.QuadPart;
	}
}

namespace Brawler
{
	Application::Application(HINSTANCE hInstance, std::int32_t nCmdShow) :
		mThreadPool(nullptr),
		mHInstance(hInstance),
		mProgramExitCode(0),
		mInitialCmdShow(nCmdShow),
		mCurrUpdateTick(0),
		mRunning(true),
		mRenderer(),
		mMainWindowManager(),
		mStateStack()
	{
		// Only allow one instance of the Application class.
		assert(appInstance == nullptr && "ERROR: A second instance of Brawler::Application was created!");
		appInstance = this;
		
		Initialize();
	}

	void Application::Initialize()
	{
		mThreadPool = std::make_unique<Brawler::WorkerThreadPool>();

		// After the call to Brawler::WorkerThreadPool::SetInitialized(), the WorkerThreads will be fully
		// unleashed. All unprotected/unsynchronized data which is to be globally read by threads *MUST*
		// have been initialized by the main thread *BEFORE* this happens!
		mThreadPool->SetInitialized();

		mRenderer.Initialize();
		AddRenderModules();

		mMainWindowManager.CreateMainWindow();
		GetMainAppWindow().ShowWindow(mInitialCmdShow);
	}

	void Application::AddRenderModules()
	{
		// Derived I_RenderModule classes should be added here in the order in which the commands they
		// generate should be executed on the GPU timeline. Keep in mind, however, that frame graph building
		// and command list recording are highly multi-threaded, so only the serial execution of the commands 
		// on the GPU timeline is guaranteed.

		mRenderer.AddRenderModule<GPUSceneUpdateRenderModule>();
	}

	void Application::Run()
	{
		const std::uint64_t perfFreq = [] ()
		{
			LARGE_INTEGER li{};
			QueryPerformanceFrequency(&li);

			return li.QuadPart;
		}();
		
		std::uint64_t startTicks = GetCurrentTickCount();
		std::uint64_t accumulator = 0;

		while (mRunning.load(std::memory_order::acquire))
		{
			const std::uint64_t frameStartTicks = GetCurrentTickCount();
			
			// Accumulate the elapsed time.
			{
				accumulator += (frameStartTicks - startTicks);
				startTicks = frameStartTicks;
			}
			
			// Perform the updates.
			while (accumulator >= PHYSICS_TIMESTEP_TICKS)
			{
				Update(PHYSICS_TIMESTEP_S);

				// If we decide to exit during the last update, then just break out
				// of the main loop immediately.
				if (!mRunning)
					break;

				accumulator -= PHYSICS_TIMESTEP_TICKS;
			}

			// Draw the current frame.
			{
				const float variableTimeStep = PHYSICS_TIMESTEP_S * (static_cast<float>(accumulator) / static_cast<float>(PHYSICS_TIMESTEP_TICKS));
				Draw(variableTimeStep);
			}

			// If applicable, lock the frame-rate.
			const std::uint32_t fpsLimit = Brawler::SettingsManager::GetInstance().GetOption<Brawler::SettingID::FRAME_RATE_LIMIT>();
			if (fpsLimit)
			{
				const std::uint64_t ticksPerFrame = static_cast<std::uint64_t>((1.0f / static_cast<float>(fpsLimit)) * perfFreq);
				std::uint64_t currTicks = GetCurrentTickCount();

				while ((currTicks - frameStartTicks) < ticksPerFrame)
					currTicks = GetCurrentTickCount();
			}
		}
	}

	WorkerThreadPool& Application::GetWorkerThreadPool()
	{
		return *mThreadPool;
	}

	const WorkerThreadPool& Application::GetWorkerThreadPool() const
	{
		return *mThreadPool;
	}

	HINSTANCE Application::GetInstanceHandle() const
	{
		return mHInstance;
	}

	D3D12::Renderer& Application::GetRenderer()
	{
		return mRenderer;
	}

	const D3D12::Renderer& Application::GetRenderer() const
	{
		return mRenderer;
	}

	ApplicationStateStack& Application::GetApplicationStateStack()
	{
		return mStateStack;
	}

	const ApplicationStateStack& Application::GetApplicationStateStack() const
	{
		return mStateStack;
	}

	AppWindow& Application::GetMainAppWindow()
	{
		return mMainWindowManager.GetMainAppWindow();
	}

	const AppWindow& Application::GetMainAppWindow() const
	{
		return mMainWindowManager.GetMainAppWindow();
	}

	std::uint64_t Application::GetCurrentUpdateTick() const
	{
		return mCurrUpdateTick;
	}

	void Application::Terminate(const std::int32_t exitCode)
	{
		mProgramExitCode.store(exitCode, std::memory_order::relaxed);
		mRunning.store(false, std::memory_order::release);
	}

	std::int32_t Application::GetExitCode() const
	{
		return mProgramExitCode.load(std::memory_order::relaxed);
	}

	void Application::PreUpdate()
	{
		// Add tasks which must be executed *BEFORE* each update tick here.

		// Update any asset dependencies. We do this only before every update tick in order to better 
		// control stuttering.
		//mAssetManager.UpdateAssetDependencies();
	}

	void Application::Update(const float dt)
	{
		// Execute any tasks which must come before an update tick. This includes things such
		// as updating asset dependencies.
		PreUpdate();
		
		mStateStack.Update(dt);

		// Once updating has finished, handle the post-tick actions.
		PostUpdate();
	}

	void Application::PostUpdate()
	{
		++mCurrUpdateTick;
	}

	void Application::PreDraw()
	{}

	void Application::Draw(const float dt)
	{
		// Execute any tasks which must come before a frame.


		// Once drawing has finished, handle the post-frame actions.
		PostDraw();
	}

	void Application::PostDraw()
	{
		mRenderer.AdvanceFrame();
	}

	Application& GetApplication()
	{
		assert(appInstance != nullptr && "ERROR: An attempt was made to get the Brawler::Application instance before it was created!");

		return *appInstance;
	}

	WorkerThreadPool& GetWorkerThreadPool()
	{
		return GetApplication().GetWorkerThreadPool();
	}

	D3D12::Renderer& GetRenderer()
	{
		return GetApplication().GetRenderer();
	}
}