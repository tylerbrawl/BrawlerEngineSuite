#include <stdexcept>
#include <span>

import Brawler.Application;
import Util.Win32;
import Util.General;
import Brawler.AppParams;

namespace
{
	Brawler::AppParams ParseCommandLine(const std::span<const char*> cmdLineArgs)
	{
		if (cmdLineArgs.size() < 3) [[unlikely]]
			throw std::runtime_error{ "ERROR: Not enough command line parameters were provided!" };

		return Brawler::AppParams{
			.InputMeshFilePath{ Util::General::StringToWString(cmdLineArgs[1]) },
			.RootOutputDirectory{ Util::General::StringToWString(cmdLineArgs[2]) }
		};
	}
}

int main(const int argc, const char* argv[])
{
	try
	{
		Util::Win32::InitializeWin32Components();

		Brawler::AppParams appParams{ ParseCommandLine(std::span<const char*>{argv, static_cast<std::size_t>(argc)}) };

		Brawler::Application app{};
		app.Run(std::move(appParams));
	}
	catch (const std::exception& e)
	{
		Util::Win32::WriteFormattedConsoleMessage(e.what(), Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
		return 1;
	}

	return 0;
}