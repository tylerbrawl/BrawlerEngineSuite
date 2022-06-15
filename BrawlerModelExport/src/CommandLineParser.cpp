module;
#include <span>
#include <string>
#include <format>
#include <optional>
#include <ranges>
#include <array>
#include <numeric>
#include <vector>
#include <filesystem>
#include <cwctype>
#include <cassert>

module Brawler.CommandLineParser;
import Util.General;
import Brawler.Win32.ConsoleFormat;

/*
Command Line Context-Free Grammar (CFG):

cmd_line_args -> executable_location options_list lod_fbx_list root_output_directory
executable_location -> "[Executable Location]"
options_list -> option options_list | option
option -> model_name_option
model_name_option -> "/ModelName" "[Model Name]"
lod_fbx_list -> fbx_file_path lod_fbx_list | fbx_file_path
fbx_file_path -> "[File Path Ending with .fbx Extension]"
root_output_directory -> "[Root Output Directory]"
*/

namespace
{
	static constexpr std::size_t INVALID_CMD_LINE_ARG_INDEX = std::numeric_limits<std::size_t>::max();
	
	static constexpr std::string_view MODEL_NAME_OPTION_STR{ "/ModelName" };

	static constexpr std::wstring_view PROGRAM_USAGE_FORMAT_STRING{
LR"(Usage: {} [Options] [Path to LOD 0 FBX] [Path to LOD 1 FBX] ... [Root Output Directory]

Command Line Switches:
	/ModelName [Model Name] - Defines the name of the model. This value is used to decide where in the root output directory files will be written to. This switch is *MANDATORY*, but it only needs to be supplied once. If it is given multiple times, then the last definition will take precedence.)"
	};
}

namespace Brawler
{
	struct CommandLineErrorInfo
	{
		/// <summary>
		/// This is a sub-span of the total command line arguments span containing the
		/// parameters which will be displayed when printing out the error message. It is
		/// used to tell the user where in the command line they went wrong.
		/// </summary>
		std::span<const char*> CmdLineContext;

		/// <summary>
		/// This indicates the index within CmdLineContext of the offending command line
		/// parameter. This parameter will be colored differently from the other parameters
		/// in CmdLineContext.
		/// 
		/// *NOTE*: This is the index within CmdLineContext, and *NOT* the command line argument
		/// index! The latter refers to indexing the argv[] passed to the main() function, which
		/// is *NOT* what this value is meant to do.
		/// </summary>
		std::size_t ErrorParameterIndex;

		/// <summary>
		/// This is the message which describes to the user why an error occurred. To conform
		/// with Brawler Engine standards, these messages should explicitly begin with the text
		/// "ERROR: " before describing the error itself.
		/// </summary>
		std::wstring ErrorMessage;
	};

	Win32::FormattedConsoleMessageBuilder CreateFormattedErrorMessageBuilder(const CommandLineErrorInfo& errorInfo)
	{
		assert(errorInfo.ErrorParameterIndex < errorInfo.CmdLineContext.size());

		Win32::FormattedConsoleMessageBuilder errorMsgBuilder{ Win32::ConsoleFormat::CRITICAL_FAILURE };
		errorMsgBuilder << errorInfo.ErrorMessage << L"\n\n\t";

		bool addSpace = false;

		// Print all of the arguments in CmdLineContext before the offending argument with
		// a normal format.
		{
			errorMsgBuilder << Win32::ConsoleFormat::NORMAL;

			for (const auto cmdLineArg : errorInfo.CmdLineContext | std::views::take(errorInfo.ErrorParameterIndex))
			{
				if (addSpace)
					errorMsgBuilder << L" ";

				errorMsgBuilder << cmdLineArg;
				addSpace = true;
			}
		}

		// Print the argument which caused the error with an error format.
		{
			errorMsgBuilder << Win32::ConsoleFormat::CRITICAL_FAILURE;

			if (addSpace)
				errorMsgBuilder << L" ";

			errorMsgBuilder << errorInfo.CmdLineContext[errorInfo.ErrorParameterIndex];
		}

		// Print all of the arguments in CmdLineContext after the offending argument with
		// a normal format.
		{
			errorMsgBuilder << Win32::ConsoleFormat::NORMAL;

			for (const auto cmdLineArg : errorInfo.CmdLineContext | std::views::drop(errorInfo.ErrorParameterIndex + 1))
				// We know at this point that we need to add space characters.
				errorMsgBuilder << L" " << cmdLineArg;
		}

		return errorMsgBuilder;
	}
}

namespace Brawler
{
	CommandLineParser::CommandLineParser(const std::span<const char*> cmdLineArgsSpan) :
		mCmdLineArgsSpan(cmdLineArgsSpan),
		mCmdLineErrorMsgBuilder(),
		mModelNameIndex(INVALID_CMD_LINE_ARG_INDEX),
		mLODFBXPathIndexArr(),
		mRootOutputDirectoryIndex(INVALID_CMD_LINE_ARG_INDEX)
	{}

	bool CommandLineParser::ParseCommandLineArguments()
	{
		// cmd_line_args -> executable_location options_list lod_fbx_list root_output_directory
		//
		// We don't actually need to care about the executable_location grammar rule, so we
		// can just skip to the second command line argument.

		assert(!mCmdLineArgsSpan.empty());

		if (mCmdLineArgsSpan.size() == 1) [[unlikely]]
		{
			Win32::FormattedConsoleMessageBuilder usageMsgBuilder{};
			usageMsgBuilder << std::format(PROGRAM_USAGE_FORMAT_STRING, Util::General::StringToWString(mCmdLineArgsSpan[0]));

			mCmdLineErrorMsgBuilder = std::move(usageMsgBuilder);

			return false;
		}

		std::size_t currIndex = 1;
		const bool wereRulesParsed = (ParseOptionsList(currIndex) && ParseLevelOfDetailFBXList(currIndex) && ParseRootOutputDirectory(currIndex));

		// There should be nothing afterwards.
		return (wereRulesParsed && (currIndex == mCmdLineArgsSpan.size()));
	}

	void CommandLineParser::PrintCommandLineErrorMessage() const
	{
		// If we are calling this function, then we probably have a message to display.

		if (mCmdLineErrorMsgBuilder.has_value()) [[likely]]
			mCmdLineErrorMsgBuilder->WriteFormattedConsoleMessage();
	}

	LaunchParams CommandLineParser::GetLaunchParameters() const
	{
		// As of writing this, every time an error occurs in parsing, a FormattedConsoleMessageBuilder is generated
		// for it.
		assert(!mCmdLineErrorMsgBuilder.has_value());

		LaunchParams launchParams{};

		assert(mModelNameIndex != INVALID_CMD_LINE_ARG_INDEX);
		launchParams.SetModelName(mCmdLineArgsSpan[mModelNameIndex]);

		assert(!mLODFBXPathIndexArr.empty());
		launchParams.SetLODCount(static_cast<std::uint32_t>(mLODFBXPathIndexArr.size()));

		std::uint32_t currIndex = 0;
		for (const auto lodFilePathIndex : mLODFBXPathIndexArr)
		{
			assert(lodFilePathIndex != INVALID_CMD_LINE_ARG_INDEX);
			launchParams.AddLODFilePath(currIndex++, mCmdLineArgsSpan[lodFilePathIndex]);
		}

		assert(mRootOutputDirectoryIndex != INVALID_CMD_LINE_ARG_INDEX);
		launchParams.SetRootOutputDirectory(mCmdLineArgsSpan[mRootOutputDirectoryIndex]);

		return launchParams;
	}

	bool CommandLineParser::ParseOptionsList(std::size_t& currIndex)
	{
		// options_list -> option options_list | option
		//
		// Note that at least one option must be specified, as the /ModelName command line
		// switch is mandatory.

		while (ParseOption(currIndex));

		// Make sure that the model name was successfully parsed.
		if (mModelNameIndex == INVALID_CMD_LINE_ARG_INDEX) [[unlikely]]
		{
			SetCommandLineError(CommandLineErrorInfo{
				.CmdLineContext{mCmdLineArgsSpan.subspan(0, (currIndex + 1))},
				.ErrorParameterIndex = currIndex,
				.ErrorMessage{L"ERROR: The model name (/ModelName) was never provided! This command line switch is *MANDATORY*!"}
			});

			return false;
		}

		return true;
	}

	bool CommandLineParser::ParseOption(std::size_t& currIndex)
	{
		// option -> model_name_option

		if (currIndex >= mCmdLineArgsSpan.size())
			return false;

		// Each command line switch must start with a "/" character, so we'll check for that
		// first.
		const std::string_view switchStr{ mCmdLineArgsSpan[currIndex] };

		if (switchStr[0] != '/')
			return false;

		// model_name_option
		if (switchStr == MODEL_NAME_OPTION_STR)
			return ParseModelNameOption(currIndex);

		// The user has specified an invalid/unrecognized option.
		SetCommandLineError(CommandLineErrorInfo{
			.CmdLineContext{mCmdLineArgsSpan.subspan(0, (currIndex + 1))},
			.ErrorParameterIndex = currIndex,
			.ErrorMessage{ std::format(L"ERROR: The command line switch \"{}\" is invalid! (For a list of valid options, please launch the Brawler Model Exporter without any command line arguments.)", Util::General::StringToWString(mCmdLineArgsSpan[currIndex])) }
		});

		return false;
	}

	bool CommandLineParser::ParseModelNameOption(std::size_t& currIndex)
	{
		// model_name_option -> "/ModelName" "[Model Name]"
		
		// We assume that this function is called only once we know that the option is correct.
		assert(mCmdLineArgsSpan[currIndex] == MODEL_NAME_OPTION_STR);

		++currIndex;

		// Make sure that a model name was specified.
		if (currIndex >= mCmdLineArgsSpan.size()) [[unlikely]]
		{
			SetCommandLineError(CommandLineErrorInfo{
				.CmdLineContext{mCmdLineArgsSpan.subspan(mCmdLineArgsSpan.size() - 1, 1)},
				.ErrorParameterIndex = 0,
				.ErrorMessage{L"ERROR: No model name was provided alongside the /ModelName command line switch!"}
			});

			return false;
		}

		const std::string_view modelNameStr{ mCmdLineArgsSpan[currIndex] };
		const std::wstring modelNameWideStr{ Util::General::StringToWString(modelNameStr) };

		// Is it even possible to receive empty command line arguments?
		assert(!modelNameStr.empty() && "ERROR: Apparently, empty command line arguments *ARE* a thing!");

		// We are going to use the provided model name to write to the file system, so we need to
		// make sure that it conforms to the Win32 file naming rules. These rules can be found
		// in the bulleted list at https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions.
		
		// Check for invalid file characters.
		{
			static constexpr auto INVALID_WIN32_FILE_CHARS_ARR{ []()
			{
				std::array<wchar_t, 41> invalidCharArr{
					L'<', L'>', L':', L'"', L'/', L'\\', L'|', L'?', L'*'
				};

				// We also need to disallow ASCII characters 0-31 (NOT the literal characters '0'-'31').
				std::iota(invalidCharArr.begin() + 9, invalidCharArr.end(), 0);

				return invalidCharArr;
			}() };

			for (const auto invalidChar : INVALID_WIN32_FILE_CHARS_ARR)
			{
				if (modelNameWideStr.contains(invalidChar)) [[unlikely]]
				{
					SetCommandLineError(CommandLineErrorInfo{
						.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex - 1, 2)},
						.ErrorParameterIndex = 1,
						.ErrorMessage{std::format(L"ERROR: The model name \"{}\" contains the invalid character '{}!'", modelNameWideStr, invalidChar)}
					});

					return false;
				}
			}
		}
		
		// Check for reserved file names.
		{
			static constexpr std::array<std::wstring_view, 22> RESERVED_WIN32_FILE_NAMES_ARR{
				L"CON", L"PRN", L"AUX", L"NUL", L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9", L"LPT1",
				L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
			};

			const std::size_t lastDotPositionInModelName = modelNameWideStr.find_last_of(L'.');
			const std::wstring_view modelNamePathWithoutExtStr{ lastDotPositionInModelName == std::string_view::npos ? modelNameWideStr : modelNameWideStr.substr(0, lastDotPositionInModelName) };

			// It's interesting that IntelliSense is warning us to make reservedFileName a const auto&.
			// Most of the time, this would be an overwhelmingly good thing. However, std::string_view is
			// small enough to make creating copies acceptable/preferable. I just thought that I would point
			// this out, since that is indeed an issue with C++ for-loops. Good on you, Microsoft!
			for (const auto reservedFileName : RESERVED_WIN32_FILE_NAMES_ARR)
			{
				if (modelNamePathWithoutExtStr == reservedFileName) [[unlikely]]
				{
					SetCommandLineError(CommandLineErrorInfo{
						.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex - 1, 2)},
						.ErrorParameterIndex = 1,
						.ErrorMessage{std::format(L"ERROR: \"{}\" is a reserved file name on Windows, and thus the specified model name is invalid!", reservedFileName.data())}
					});

					return false;
				}
			}
		}

		// Ensure that the model name does not end in either a period or a space character.
		{
			if (modelNameStr.back() == '.') [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex - 1, 2)},
					.ErrorParameterIndex = 1,
					.ErrorMessage{L"ERROR: Windows does not allow files or directories to end with a period ('.') character, and thus the specified model name is invalid!"}
				});

				return false;
			}

			if (modelNameStr.back() == ' ') [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex - 1, 2)},
					.ErrorParameterIndex = 1,
					.ErrorMessage{L"ERROR: Windows does not allow files or directories to end with a space (' ') character, and thus the specified model name is invalid!"}
				});

				return false;
			}
		}

		// The model name appears to have passed all of the checks listed on the MSDN, so we accept
		// it.
		mModelNameIndex = currIndex++;
		return true;
	}

	bool CommandLineParser::ParseLevelOfDetailFBXList(std::size_t& currIndex)
	{
		// lod_fbx_list -> fbx_file_path lod_fbx_list | fbx_file_path
		//
		// Note that at least one .fbx file must be specified.

		if (!ParseFBXFilePath(currIndex)) [[unlikely]]
		{
			SetCommandLineError(CommandLineErrorInfo{
				.CmdLineContext{mCmdLineArgsSpan.subspan(mCmdLineArgsSpan.size() - 2, 2)},
				.ErrorParameterIndex = 1,
				.ErrorMessage{L"ERROR: No input FBX files were specified!"}
			});

			return false;
		}

		while (ParseFBXFilePath(currIndex));
		return true;
	}

	bool CommandLineParser::ParseFBXFilePath(std::size_t& currIndex)
	{
		// fbx_file_path -> "[File Path Ending with .fbx Extension]"

		if (currIndex >= mCmdLineArgsSpan.size()) [[unlikely]]
			return false;

		const std::filesystem::path fbxFilePath{ mCmdLineArgsSpan[currIndex] };

		// Ensure that the extension is of type FBX. Technically, the correct thing to do would
		// be to check the magic header of the file. Since this is just a tool, however, we won't
		// bother.
		{
			const std::filesystem::path fbxExtensionPath{ fbxFilePath.extension() };
			std::wstring fbxExtensionString{ fbxExtensionPath.wstring() };

			std::ranges::transform(fbxExtensionString, fbxExtensionString.begin(), [] (const wchar_t c) { return std::towlower(c); });

			// It's not necessarily an error if the next file is not a .fbx file. It could mean
			// that we have reached the end of the input files.
			if (fbxExtensionString != L".fbx") [[unlikely]]
				return false;
		}

		// Check that the specified file actually exists.
		{
			std::error_code errorCode{};
			const bool doesFileExist = std::filesystem::exists(fbxFilePath, errorCode);
			
			if (errorCode) [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
					.ErrorParameterIndex = 0,
					.ErrorMessage{std::format(L"ERROR: The attempt to verify that the input FBX file \"{}\" exists failed with the following error: {}", fbxFilePath.c_str(), Util::General::StringToWString(errorCode.message()))}
				});

				return false;
			}

			if (!doesFileExist) [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
					.ErrorParameterIndex = 0,
					.ErrorMessage{std::format(L"ERROR: The input FBX file \"{}\" does not exist!", fbxFilePath.c_str())}
				});

				return false;
			}

			const bool isDirectory = std::filesystem::is_directory(fbxFilePath, errorCode);

			if (errorCode) [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
					.ErrorParameterIndex = 0,
					.ErrorMessage{std::format(L"ERROR: The attempt to verify whether the input FBX file \"{}\" was actually a directory failed with the following error: {}", fbxFilePath.c_str(), Util::General::StringToWString(errorCode.message()))}
				});

				return false;
			}

			if (isDirectory) [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
					.ErrorParameterIndex = 0,
					.ErrorMessage{std::format(L"ERROR: The specified input FBX file \"{}\" is actually a directory!", fbxFilePath.c_str())}
				});

				return false;
			}
		}

		// The specified file appears to actually be an FBX file which we can/will use.
		mLODFBXPathIndexArr.push_back(currIndex++);
		return true;
	}

	bool CommandLineParser::ParseRootOutputDirectory(std::size_t& currIndex)
	{
		// root_output_directory -> "[Root Output Directory]"

		if (currIndex >= mCmdLineArgsSpan.size()) [[unlikely]]
		{
			SetCommandLineError(CommandLineErrorInfo{
				.CmdLineContext{mCmdLineArgsSpan.subspan(mCmdLineArgsSpan.size() - 2, 2)},
				.ErrorParameterIndex = 1,
				.ErrorMessage{L"ERROR: No root output directory was specified!"}
			});

			return false;
		}

		const std::filesystem::path rootOutputDirectory{ mCmdLineArgsSpan[currIndex] };

		{
			std::error_code errorCode{};
			const bool doesPathExist = std::filesystem::exists(rootOutputDirectory, errorCode);

			if (errorCode) [[unlikely]]
			{
				SetCommandLineError(CommandLineErrorInfo{
					.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
					.ErrorParameterIndex = 0,
					.ErrorMessage{std::format(L"ERROR: The attempt to check if the root output directory \"{}\" exists failed with the following error: {}", rootOutputDirectory.c_str(), Util::General::StringToWString(errorCode.message()))}
				});

				return false;
			}

			// If this path already exists, then ensure that it refers to a directory.
			if (doesPathExist)
			{
				const bool isDirectory = std::filesystem::is_directory(rootOutputDirectory, errorCode);

				if (errorCode) [[unlikely]]
				{
					SetCommandLineError(CommandLineErrorInfo{
						.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
						.ErrorParameterIndex = 0,
						.ErrorMessage{std::format(L"ERROR: The attempt to check if the existing root output directory \"{}\" is actually a directory failed with the following error: {}", rootOutputDirectory.c_str(), Util::General::StringToWString(errorCode.message()))}
					});

					return false;
				}

				if (!isDirectory) [[unlikely]]
				{
					SetCommandLineError(CommandLineErrorInfo{
						.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
						.ErrorParameterIndex = 0,
						.ErrorMessage{std::format(L"ERROR: The specified root output directory \"{}\" exists, but is not actually a directory!", rootOutputDirectory.c_str())}
					});

					return false;
				}
			}

			// If this path does *NOT* already exist, then ensure that we can create the directory.
			else
			{
				const bool wasDirectoryCreated = std::filesystem::create_directories(rootOutputDirectory, errorCode);

				if (errorCode) [[unlikely]]
				{
					SetCommandLineError(CommandLineErrorInfo{
						.CmdLineContext{mCmdLineArgsSpan.subspan(currIndex, 1)},
						.ErrorParameterIndex = 0,
						.ErrorMessage{std::format(L"ERROR: The program successfully verified that the root output directory \"{}\" does not exist, but attempting to create the directory failed with the following error: {}", rootOutputDirectory.c_str(), Util::General::StringToWString(errorCode.message()))}
					});

					return false;
				}
			}
		}

		// The specified root output directory appears to be a valid directory, so we
		// accept it.
		mRootOutputDirectoryIndex = currIndex++;
		return true;
	}

	void CommandLineParser::SetCommandLineError(const CommandLineErrorInfo& errorInfo)
	{
		if (!mCmdLineErrorMsgBuilder.has_value())
			mCmdLineErrorMsgBuilder = CreateFormattedErrorMessageBuilder(errorInfo);
	}
}