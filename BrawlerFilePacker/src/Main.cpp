#include <stdexcept>
#include <iostream>
#include <sstream>
#include <array>

#pragma warning(push)
#pragma warning(disable: 5105)
#include "Win32Def.h"
#pragma warning(pop)

import Brawler.Application;
import Brawler.ExceptionReporter;
import Brawler.PackerSettings;
import Util.General;
import Util.Win32;
import Brawler.AppParams;

namespace
{
	std::string GetUsageInformation()
	{
		std::string strStream{ "Usage: BrawlerFilePacker.exe" };

		// After the executable, the root directory of a project's data files should be specified.
		strStream += " [Root Directory of Project Data Files]";

		// Next, the root directory of the output should be specified.
		strStream += " [Root Directory of Output Files]";

		// After that comes a set of switches.
		strStream += " [Additional Parameters (Optional)]\n\nParameter List:\n";

		for (const auto& switchDesc : Brawler::PackerSettings::SWITCH_DESCRIPTION_ARR)
			strStream += std::string{ "\t" + std::string{ switchDesc.CmdLineSwitch } + "\t\t" + std::string{ switchDesc.Description } + "\n" };

		return strStream;
	}
}


int main(int argc, char* argv[])
{
	try
	{
		Util::Win32::EnableConsoleFormatting();
		
		if (argc < 3)
		{
			Util::Win32::WriteFormattedConsoleMessage(GetUsageInformation());
			return 1;
		}

		const std::string_view rootDataDirectory{ argv[1] };
		const std::string_view rootOutputDirectory{ argv[2] };

		std::uint64_t switchBitMask = 0;
		for (std::size_t i = 3; i < static_cast<std::size_t>(argc); ++i)
		{
			// Check every possible switch for this command line argument.
			for (const auto& switchDesc : Brawler::PackerSettings::SWITCH_DESCRIPTION_ARR)
			{
				if (!std::strcmp(argv[i], switchDesc.CmdLineSwitch))
				{
					switchBitMask |= static_cast<std::uint64_t>(switchDesc.SwitchID);
					break;
				}
			}
		}
		
		// Why do I need to suppress warnings here of all places?!
#pragma warning(push)
#pragma warning(disable: 4005)
#pragma warning(disable: 5106)
		Brawler::Application app{};
		app.Run(Brawler::AppParams{ rootDataDirectory, rootOutputDirectory, switchBitMask });
#pragma warning(pop)
	}
	catch (const std::exception& e)
	{
		Brawler::ExceptionReporter reporter{ e };
		reporter.ReportException();

		return 1;
	}

	return 0;
}