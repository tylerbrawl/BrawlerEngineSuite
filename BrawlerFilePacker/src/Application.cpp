module;
#include <cassert>
#include <string>
#include <stdexcept>
#include <thread>

module Brawler.Application;
import Brawler.PackerSettings;
import Brawler.WorkerThreadPool;
import Brawler.HashProvider;
import Brawler.AssetCompiler;
import Brawler.AssetCompilerContext;
import Brawler.AppParams;
import Util.General;
import Util.Win32;

namespace
{
	static Brawler::Application* appInstance = nullptr;
}

namespace Brawler
{
	Application::Application() :
		mBuildMode(PackerSettings::BuildMode::RELEASE),  // We will default to Release builds.
		mHashProvider(),
		mThreadPool(),
		mAssetCompiler()
	{
		assert(appInstance == nullptr && "ERROR: An attempt was made to create a second Brawler::Application!");
		appInstance = this;
		
		Initialize();
	}

	void Application::Initialize()
	{
		mThreadPool.SetInitialized();
	}

	Application& Application::GetInstance()
	{
		assert(appInstance != nullptr);
		return *appInstance;
	}

	void Application::Run(const AppParams& appParams)
	{
		// Select the correct BuildMode. We don't really have to check explicitly for /R, since
		// that is the assumed default.

		if (appParams.SwitchBitMask & static_cast<std::uint64_t>(PackerSettings::FilePackerSwitchID::BUILD_FOR_DEBUG))
		{
			// Make sure that /R wasn't also provided.
			if (appParams.SwitchBitMask & static_cast<std::uint64_t>(PackerSettings::FilePackerSwitchID::BUILD_FOR_RELEASE))
				throw std::runtime_error{ "ERROR: The /D switch is mutually exclusive with the /R switch!" };

			mBuildMode = PackerSettings::BuildMode::DEBUG;
		}

		const AssetCompilerContext context{ mBuildMode, std::filesystem::path{Util::General::StringToWString(appParams.RootDataDirectory)}, std::filesystem::path{Util::General::StringToWString(appParams.RootOutputDirectory)} };
		mAssetCompiler.BeginAssetCompilationPipeline(context);
	}

	PackerSettings::BuildMode Application::GetAssetBuildMode() const
	{
		return mBuildMode;
	}

	WorkerThreadPool& Application::GetWorkerThreadPool()
	{
		return mThreadPool;
	}

	const WorkerThreadPool& Application::GetWorkerThreadPool() const
	{
		return mThreadPool;
	}

	HashProvider& Application::GetHashProvider()
	{
		return mHashProvider;
	}

	const HashProvider& Application::GetHashProvider() const
	{
		return mHashProvider;
	}
}