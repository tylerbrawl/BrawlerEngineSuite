module;
#include <cassert>
#include <fstream>
#include <span>
#include <array>

module Brawler.Application;
import Util.FileWrite;
import Brawler.DLLManager;
import Util.Win32;
import Brawler.ShaderProfileID;
import Brawler.ShaderProfileDefinition;

namespace
{
	static Brawler::Application* appPtr = nullptr;
}

namespace Brawler
{
	Application::Application(AppParams&& appParams) :
		mAppParams(std::move(appParams)),
		mThreadPool()
	{
		assert(appPtr == nullptr && "ERROR: An attempt was made to create a second instance of Brawler::Application!");
		appPtr = this;

		Initialize();
	}

	void Application::Initialize()
	{
		mThreadPool.SetInitialized();
	}

	void Application::Run()
	{
		Util::Win32::WriteFormattedConsoleMessage("Generating shader compiler files...");
		Util::FileWrite::SerializeSourceFiles();

		PrintCompletionDiagnostics();
	}

	WorkerThreadPool& Application::GetWorkerThreadPool()
	{
		return mThreadPool;
	}

	const WorkerThreadPool& Application::GetWorkerThreadPool() const
	{
		return mThreadPool;
	}

	const AppParams& Application::GetLaunchParameters() const
	{
		return mAppParams;
	}

	void Application::PrintCompletionDiagnostics() const
	{
		const std::string_view completionDiagnosticsStr{ [] <ShaderProfiles::ShaderProfileID CurrProfileID>(this const auto& self, const ShaderProfiles::ShaderProfileID profileID)
		{
			if constexpr (CurrProfileID != ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR)
			{
				if (CurrProfileID == profileID)
				{
					static constexpr std::string_view CURR_COMPLETION_DIAGNOSTICS_STR{ ShaderProfiles::GetCompletionDiagnosticsString<CurrProfileID>() };
					return CURR_COMPLETION_DIAGNOSTICS_STR;
				}

				constexpr ShaderProfiles::ShaderProfileID NEXT_ID = static_cast<ShaderProfiles::ShaderProfileID>(std::to_underlying(CurrProfileID) + 1);
				return self.template operator()<NEXT_ID>(profileID);
			}
			else
			{
				assert(false);
				std::unreachable();

				return std::string_view{};
			}
		}.template operator()<static_cast<ShaderProfiles::ShaderProfileID>(0)>(mAppParams.ShaderProfile) };

		Util::Win32::WriteFormattedConsoleMessage(completionDiagnosticsStr, Util::Win32::ConsoleFormat::SUCCESS);
	}

	Application& GetApplication()
	{
		assert(appPtr != nullptr && "ERROR: Brawler::GetApplication() was called before the static Brawler::Application pointer could be initialized!");
		return *appPtr;
	}
}