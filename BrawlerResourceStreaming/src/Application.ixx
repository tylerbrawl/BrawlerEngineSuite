module;
#include <thread>

export module Brawler.Application;
import Brawler.WorkerThreadPool;
import Brawler.AssetManager;
import Brawler.AudioManager;

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
		void Run();

	private:
		void ExecutePreUpdateTasks();
		void Update(const float dt);

	public:
		WorkerThreadPool& GetWorkerThreadPool();
		const WorkerThreadPool& GetWorkerThreadPool() const;

		AssetManager& GetAssetManager();
		const AssetManager& GetAssetManager() const;

		AudioManager& GetAudioManager();
		const AudioManager& GetAudioManager() const;

		static Application& GetInstance();

	private:
		WorkerThreadPool mThreadPool;
		AssetManager mAssetManager;
		AudioManager mAudioManager;
	};
}