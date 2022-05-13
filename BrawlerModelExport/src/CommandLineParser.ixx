module;
#include <span>
#include <string>
#include <optional>
#include <vector>

export module Brawler.CommandLineParser;
import Brawler.Win32.FormattedConsoleMessageBuilder;
import Brawler.LaunchParams;

namespace Brawler
{
	struct CommandLineErrorInfo;
}

export namespace Brawler
{
	class CommandLineParser
	{
	public:
		CommandLineParser() = default;
		explicit CommandLineParser(const std::span<const char*> cmdLineArgsSpan);

		CommandLineParser(const CommandLineParser& rhs) = delete;
		CommandLineParser& operator=(const CommandLineParser& rhs) = delete;

		CommandLineParser(CommandLineParser&& rhs) noexcept = default;
		CommandLineParser& operator=(CommandLineParser&& rhs) noexcept = default;

		/// <summary>
		/// Parses the command line arguments specified in the constructor of this
		/// CommandLineParser instance for validity. If an error occurs during parsing,
		/// then this function will return false and CommandLineParser::ParseCommandLineArguments()
		/// may have an error message to print to the console. Otherwise, this function
		/// will return true, and calling CommandLineParser::PrintCommandLineErrorMessage()
		/// will be redundant.
		/// </summary>
		/// <returns>
		/// The function returns true if the command line arguments specified in the constructor
		/// of this CommandLineParser instance were deemed valid and false otherwise. 
		/// 
		/// Iff this function returns false, then CommandLineParser::PrintCommandLineErrorMessage()
		/// may have a message to print to STDOUT for the user. In that case, the function
		/// should be called before terminating the program.
		/// </returns>
		bool ParseCommandLineArguments();

		/// <summary>
		/// If the command line arguments were deemed invalid, then calling this function may
		/// print a message to STDOUT explaining why the commands were invalid. It only makes
		/// sense to call this function if CommandLineParser::ParseCommandLineArguments()
		/// returns false.
		/// </summary>
		void PrintCommandLineErrorMessage() const;

		LaunchParams GetLaunchParameters() const;

	private:
		bool ParseOptionsList(std::size_t& currIndex);
		bool ParseOption(std::size_t& currIndex);
		bool ParseModelNameOption(std::size_t& currIndex);
		bool ParseLevelOfDetailFBXList(std::size_t& currIndex);
		bool ParseFBXFilePath(std::size_t& currIndex);
		bool ParseRootOutputDirectory(std::size_t& currIndex);

		void SetCommandLineError(const CommandLineErrorInfo& errorInfo);

	private:
		std::span<const char*> mCmdLineArgsSpan;
		std::optional<Win32::FormattedConsoleMessageBuilder> mCmdLineErrorMsgBuilder;
		std::size_t mModelNameIndex;
		std::vector<std::size_t> mLODFBXPathIndexArr;
		std::size_t mRootOutputDirectoryIndex;
	};
}