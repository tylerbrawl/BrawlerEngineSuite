module;
#include <memory>
#include "DxDef.h"

export module Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.D3D12.Renderer;
import Brawler.ApplicationStateStack;

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

	public:
		void Run();

		WorkerThreadPool& GetWorkerThreadPool();
		const WorkerThreadPool& GetWorkerThreadPool() const;

		HINSTANCE GetInstanceHandle() const;
		std::int32_t GetInitialCmdShow() const;

		D3D12::Renderer& GetRenderer();
		const D3D12::Renderer& GetRenderer() const;

		std::uint64_t GetCurrentUpdateTick() const;

		void Terminate();

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
		const std::int32_t mInitialCmdShow;
		std::uint64_t mCurrUpdateTick;
		bool mRunning;
		D3D12::Renderer mRenderer;
		ApplicationStateStack mStateStack;
	};

	Application& GetApplication();
	WorkerThreadPool& GetWorkerThreadPool();
	D3D12::Renderer& GetRenderer();
}