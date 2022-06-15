#include <stdexcept>
#include <span>
#include <optional>

import Brawler.Application;
import Util.Win32;
import Util.General;
import Brawler.LaunchParams;
import Brawler.CommandLineParser;
import Brawler.Win32.ConsoleFormat;

namespace
{
	std::optional<Brawler::LaunchParams> ParseCommandLine(const std::span<const char*> cmdLineArgs)
	{
		Brawler::CommandLineParser cmdLineParser{ cmdLineArgs };

		if (!cmdLineParser.ParseCommandLineArguments()) [[unlikely]]
		{
			cmdLineParser.PrintCommandLineErrorMessage();
			return std::optional<Brawler::LaunchParams>{};
		}

		return std::optional<Brawler::LaunchParams>{ cmdLineParser.GetLaunchParameters() };
	}
}

int main(const int argc, const char* argv[])
{
	try
	{
		Util::Win32::InitializeWin32Components();

		std::optional<Brawler::LaunchParams> launchParams{ ParseCommandLine(std::span<const char*>{argv, static_cast<std::size_t>(argc)}) };

		if (!launchParams.has_value()) [[unlikely]]
			return 1;

		Brawler::Application app{};
		app.Run(std::move(*launchParams));
	}
	catch (const std::exception& e)
	{
		Util::Win32::WriteFormattedConsoleMessage(e.what(), Brawler::Win32::ConsoleFormat::CRITICAL_FAILURE);
		return 1;
	}

	return 0;
}