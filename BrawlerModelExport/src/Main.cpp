#include <stdexcept>
#include <span>
#include <optional>

import Brawler.Application;
import Util.Win32;
import Util.General;
import Brawler.AppParams;
import Brawler.CommandLineParser;
import Brawler.Win32.ConsoleFormat;

namespace
{
	std::optional<Brawler::AppParams> ParseCommandLine(const std::span<const char*> cmdLineArgs)
	{
		Brawler::CommandLineParser cmdLineParser{ cmdLineArgs };

		if (!cmdLineParser.ParseCommandLineArguments()) [[unlikely]]
		{
			cmdLineParser.PrintCommandLineErrorMessage();
			return std::optional<Brawler::AppParams>{};
		}

		return std::optional<Brawler::AppParams>{ cmdLineParser.GetLaunchParameters() };
	}
}

int main(const int argc, const char* argv[])
{
	try
	{
		Util::Win32::InitializeWin32Components();

		std::optional<Brawler::AppParams> appParams{ ParseCommandLine(std::span<const char*>{argv, static_cast<std::size_t>(argc)}) };

		if (!appParams.has_value()) [[unlikely]]
			return 1;

		Brawler::Application app{};
		app.Run(std::move(*appParams));
	}
	catch (const std::exception& e)
	{
		Util::Win32::WriteFormattedConsoleMessage(e.what(), Brawler::Win32::ConsoleFormat::CRITICAL_FAILURE);
		return 1;
	}

	return 0;
}