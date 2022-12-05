module;
#include <memory>
#include <atomic>
#include "DxDef.h"

export module Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.D3D12.Renderer;
import Brawler.ApplicationStateStack;
import Brawler.AppWindow;
import Brawler.MainWindowManager;

export namespace Brawler
{
	class Application
	{
	public:
		Application(HINSTANCE hInstance, std::int32_t nCmdShow);

		Application(const Application& rhs) = delete;
		Application& operator=(const Application& rhs) = delete;

		Application(Application&& rhs) noexcept = default;
		Application& operator=(Application&& rhs) noexcept = default;

	private:
		void Initialize();
		void AddRenderModules();

	public:
		void Run();

		WorkerThreadPool& GetWorkerThreadPool();
		const WorkerThreadPool& GetWorkerThreadPool() const;

		HINSTANCE GetInstanceHandle() const;
		std::int32_t GetInitialCmdShow() const;

		D3D12::Renderer& GetRenderer();
		const D3D12::Renderer& GetRenderer() const;

		AppWindow& GetMainAppWindow();
		const AppWindow& GetMainAppWindow() const;

		std::uint64_t GetCurrentUpdateTick() const;

		void Terminate(const std::int32_t exitCode = 0);

		std::int32_t GetExitCode() const;

	private:
		void PreUpdate();
		void Update(const float dt);
		void PostUpdate();

		void PreDraw();
		void Draw(const float dt);
		void PostDraw();

	private:
		std::unique_ptr<WorkerThreadPool> mThreadPool;
		HINSTANCE mHInstance;
		std::atomic<std::int32_t> mProgramExitCode;
		const std::int32_t mInitialCmdShow;
		std::uint64_t mCurrUpdateTick;
		std::atomic<bool> mRunning;
		D3D12::Renderer mRenderer;
		MainWindowManager mMainWindowManager;
		ApplicationStateStack mStateStack;
	};

	Application& GetApplication();
	WorkerThreadPool& GetWorkerThreadPool();
	D3D12::Renderer& GetRenderer();
}