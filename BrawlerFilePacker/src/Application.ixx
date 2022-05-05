module;
#include <string>
#include <thread>

export module Brawler.Application;
import Brawler.PackerSettings;
import Brawler.WorkerThreadPool;
import Brawler.HashProvider;
import Brawler.AssetCompiler;

export namespace Brawler
{
	struct AppParams;
}

export namespace Brawler
{
	class Application
	{
	public:
		Application();

		Application(const Application& rhs) = delete;
		Application& operator=(const Application& rhs) = delete;

		Application(Application&& rhs) noexcept = default;
		Application& operator=(Application&& rhs) noexcept = default;

	private:
		void Initialize();

	public:
		static Application& GetInstance();
		void Run(const AppParams& appParams);

		PackerSettings::BuildMode GetAssetBuildMode() const;

		WorkerThreadPool& GetWorkerThreadPool();
		const WorkerThreadPool& GetWorkerThreadPool() const;

		HashProvider& GetHashProvider();
		const HashProvider& GetHashProvider() const;

	private:
		PackerSettings::BuildMode mBuildMode;
		Brawler::HashProvider mHashProvider;
		Brawler::WorkerThreadPool mThreadPool;
		Brawler::AssetCompiler mAssetCompiler;
	};
}