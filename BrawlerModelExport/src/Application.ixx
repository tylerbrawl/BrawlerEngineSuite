module;
#include <memory>
#include <assimp/scene.h>

export module Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.ModelResolver;
import Brawler.AppParams;
import Brawler.D3D12.Renderer;

export namespace Brawler
{
	class Application
	{
	public:
		Application();

	private:
		void Initialize();

	public:
		void Run();

		void SetLaunchParameters(AppParams&& appParams);
		const AppParams& GetLaunchParameters() const;

		WorkerThreadPool& GetWorkerThreadPool();
		const WorkerThreadPool& GetWorkerThreadPool() const;

		D3D12::Renderer& GetRenderer();
		const D3D12::Renderer& GetRenderer() const;

	private:
		WorkerThreadPool mThreadPool;
		D3D12::Renderer mRenderer;
		ModelResolver mModelResolver;
		AppParams mLaunchParams;
	};

	Application& GetApplication();
	WorkerThreadPool& GetWorkerThreadPool();
	D3D12::Renderer& GetRenderer();
}