module;
#include <memory>
#include <assimp/scene.h>

export module Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.ModelResolver;
import Brawler.LaunchParams;
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
		void Run(LaunchParams&& launchParams);

		const LaunchParams& GetLaunchParameters() const;

		WorkerThreadPool& GetWorkerThreadPool();
		const WorkerThreadPool& GetWorkerThreadPool() const;

		D3D12::Renderer& GetRenderer();
		const D3D12::Renderer& GetRenderer() const;

	private:
		void ExecuteModelConversionLoop();

	private:
		WorkerThreadPool mThreadPool;
		D3D12::Renderer mRenderer;
		ModelResolver mModelResolver;
		LaunchParams mLaunchParams;
	};

	Application& GetApplication();
	WorkerThreadPool& GetWorkerThreadPool();
	D3D12::Renderer& GetRenderer();
}