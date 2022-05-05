module;
#include <cassert>
#include <thread>
#include <memory>
#include "Win32Def.h"

module Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.AssetSystem;
import Brawler.StreamedAudioVoice;
import Util.Win32;

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

	std::unique_ptr<Brawler::StreamedAudioVoice> streamedAudioVoice{ nullptr };

	__forceinline std::uint64_t GetCurrentTickCount()
	{
		LARGE_INTEGER li{};
		QueryPerformanceCounter(&li);

		return li.QuadPart;
	}
}

namespace Brawler
{
	Application::Application() :
		mThreadPool(),
		mAssetManager(),
		mAudioManager()
	{
		assert(appInstance == nullptr && "ERROR: An attempt was made to construct a second instance of Brawler::Application!");
		appInstance = this;

		Initialize();
	}

	void Application::Initialize()
	{
		Util::Win32::EnableConsoleFormatting();
		Util::Win32::InitializeCOM();

		mAudioManager.Initialize();

		AssetHandle<StreamedAudio> hAudioAsset = mAssetManager.CreateAssetHandle<StreamedAudio>(std::wstring_view{ L"Music\\Providence.wav" });

		AssetDependencyGroup dependencyGroup{};
		dependencyGroup.AddAssetDependency(hAudioAsset);
		mAssetManager.SubmitAssetDependencyGroup(std::move(dependencyGroup));

		streamedAudioVoice = std::make_unique<StreamedAudioVoice>(hAudioAsset);
		
		mThreadPool.SetInitialized();
	}

	void Application::Run()
	{
		std::uint64_t startTicks = GetCurrentTickCount();
		std::uint64_t accumulator = 0;

		while (true)
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
				ExecutePreUpdateTasks();
				Update(PHYSICS_TIMESTEP_MS);

				accumulator -= PHYSICS_TIMESTEP_TICKS;
			}
		}
	}

	void Application::ExecutePreUpdateTasks()
	{
		// Before each update, we need to update the asset dependencies in the AssetManager.
		mAssetManager.UpdateAssetDependencies();
	}

	void Application::Update(const float dt)
	{
		streamedAudioVoice->Update(dt);
	}

	WorkerThreadPool& Application::GetWorkerThreadPool()
	{
		return mThreadPool;
	}

	const WorkerThreadPool& Application::GetWorkerThreadPool() const
	{
		return mThreadPool;
	}

	AssetManager& Application::GetAssetManager()
	{
		return mAssetManager;
	}

	const AssetManager& Application::GetAssetManager() const
	{
		return mAssetManager;
	}

	AudioManager& Application::GetAudioManager()
	{
		return mAudioManager;
	}

	const AudioManager& Application::GetAudioManager() const
	{
		return mAudioManager;
	}

	Application& Application::GetInstance()
	{
		assert(appInstance != nullptr && "ERROR: Application::GetInstance() was called before the static pointer could be initialized!");
		return *appInstance;
	}
}