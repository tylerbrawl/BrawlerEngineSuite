module;
#include <string>
#include <vector>
#include <filesystem>

export module Brawler.ShaderCompilationParams;

export namespace Brawler
{
	struct HLSLPreprocessorMacro
	{
		std::wstring_view MacroName;
		std::wstring_view MacroValue;
	};

	struct ShaderCompilationParams
	{
		/// <summary>
		/// This is the file path to the shader file which is to be compiled. It should
		/// be relative to the root source directory specified as a command line argument.
		/// </summary>
		std::filesystem::path FilePath;

		/// <summary>
		/// This is the name of the entry point of the shader program.
		/// </summary>
		std::wstring_view EntryPoint;

		/// <summary>
		/// This is a std::vector containing all of the preprocessor defines which will
		/// be added during shader compilation.
		/// </summary>
		std::vector<HLSLPreprocessorMacro> MacroDefinitionArr;
	};
}