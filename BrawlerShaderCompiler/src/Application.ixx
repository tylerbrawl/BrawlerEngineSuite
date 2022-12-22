module;
#include <compare>  // Classic MSVC modules jank...

export module Brawler.Application;
import Brawler.AppParams;
import Brawler.WorkerThreadPool;

export namespace Brawler
{
	class Application
	{
	public:
		explicit Application(AppParams&& appParams);

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

		const AppParams& GetLaunchParameters() const;

	private:
		void PrintCompletionDiagnostics() const;

	private:
		AppParams mAppParams;
		WorkerThreadPool mThreadPool;
	};

	Application& GetApplication();
}