#include <stdexcept>
#include <span>
#include <string>
#include <array>
#include <filesystem>
#include <ranges>
#include "DxDef.h"

import Brawler.Application;
import Brawler.AppParams;
import Brawler.ShaderProfileID;
import Brawler.ShaderProfileDefinition;
import Util.Win32;

namespace
{
	struct CommandLineArgumentDescription
	{
		std::string_view SwitchStr;
		std::string_view SwitchDescription;
	};

	static constexpr std::array<CommandLineArgumentDescription, 1> CMD_LINE_ARG_DESCRIPTION_ARR{
		{"/P:[Shader Profile]", "Specifies the shader profile. The shader profile decides which PSOs and root signatures are serialized for the current execution of the program. A list of valid shader profiles is given following the list of available command line options. This setting is *MANDATORY*."}
	};

	template <Brawler::ShaderProfiles::ShaderProfileID CurrProfileID>
	void AddShaderProfileDescriptionToString(std::string& profileDescStr)
	{
		profileDescStr += Brawler::ShaderProfiles::GetCommandLineSelectionString<CurrProfileID>();
		profileDescStr += "\t\t";
		profileDescStr += Brawler::ShaderProfiles::GetCommandLineDescriptionString<CurrProfileID>();
		profileDescStr += "\n";

		AddShaderProfileDescriptionToString<static_cast<Brawler::ShaderProfiles::ShaderProfileID>(std::to_underlying(CurrProfileID) + 1)>(profileDescStr);
	}

	template <>
	void AddShaderProfileDescriptionToString<Brawler::ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR>(std::string& profileDescStr)
	{}

	void PrintUsageInformation(const std::string_view executablePath)
	{
		{
			std::string usageStr{ "Usage: " };
			usageStr += executablePath;
			usageStr += " [Root Source File Directory] [Options]\n";

			Util::Win32::WriteFormattedConsoleMessage(usageStr, Util::Win32::ConsoleFormat::NORMAL);
		}

		{
			std::string optionsStr{ "Available Options:\n" };

			for (const auto& argDesc : CMD_LINE_ARG_DESCRIPTION_ARR)
				optionsStr += std::string{ argDesc.SwitchStr } + "\t\t" + std::string{ argDesc.SwitchDescription } + "\n";

			Util::Win32::WriteFormattedConsoleMessage(optionsStr, Util::Win32::ConsoleFormat::NORMAL);
		}

		{
			std::string shaderProfilesStr{ "Shader Profiles:\n" };
			AddShaderProfileDescriptionToString<static_cast<Brawler::ShaderProfiles::ShaderProfileID>(0)>(shaderProfilesStr);

			Util::Win32::WriteFormattedConsoleMessage(shaderProfilesStr, Util::Win32::ConsoleFormat::NORMAL);
		}
	}

	template <Brawler::ShaderProfiles::ShaderProfileID CurrProfileID>
	Brawler::ShaderProfiles::ShaderProfileID GetShaderProfileIDFromString(const std::string_view profileIDStr)
	{
		if (profileIDStr == Brawler::ShaderProfiles::GetCommandLineSelectionString<CurrProfileID>())
			return CurrProfileID;

		return GetShaderProfileIDFromString<static_cast<Brawler::ShaderProfiles::ShaderProfileID>(std::to_underlying(CurrProfileID) + 1)>(profileIDStr);
	}

	template <>
	Brawler::ShaderProfiles::ShaderProfileID GetShaderProfileIDFromString<Brawler::ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR>(const std::string_view profileIDStr)
	{
		return Brawler::ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR;
	}

	Brawler::AppParams GetLaunchParameters(std::span<const char* const> cmdLineArgs)
	{
		// If no arguments are specified, then print out the usage information and exit
		// the program.
		if (cmdLineArgs.size() == 1) [[unlikely]]
		{
			PrintUsageInformation(cmdLineArgs[0]);
			throw std::runtime_error{""};
		}
		
		Brawler::AppParams appParams{
			.RootSourceDirectory{cmdLineArgs[1]},
			.ShaderProfile = Brawler::ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR
		};

		{
			std::error_code errorCode{};
			const bool isSpecifiedPathDirectory = std::filesystem::is_directory(appParams.RootSourceDirectory, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::string{ "ERROR: The attempt to verify that "} + cmdLineArgs[1] + " is a directory failed with the following error: " + errorCode.message() };
		}

		for (const auto& optionCStr : cmdLineArgs | std::views::drop(2))
		{
			const std::string_view cmdLineOption{ optionCStr };

			if (cmdLineOption.substr(0, 3) == "/P:")
				appParams.ShaderProfile = GetShaderProfileIDFromString<static_cast<Brawler::ShaderProfiles::ShaderProfileID>(0)>(cmdLineOption.substr(3));
		}

		// Verify that a valid shader profile was chosen.
		if (appParams.ShaderProfile == Brawler::ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR) [[unlikely]]
			throw std::runtime_error{ "ERROR: A valid shader profile ID was not provided! (Launching this program without any additional arguments will provide a list of valid shader profiles.)" };

		return appParams;
	}
}

int main(const int argc, const char* argv[])
{
	try
	{
		Util::Win32::InitializeWin32Components();

		Brawler::Application app{ GetLaunchParameters(std::span<const char* const>{ argv, static_cast<std::size_t>(argc) }) };
		app.Run();
	}
	catch (const std::exception& e)
	{
		Util::Win32::WriteFormattedConsoleMessage(e.what(), Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
		return 1;
	}

	return 0;
}