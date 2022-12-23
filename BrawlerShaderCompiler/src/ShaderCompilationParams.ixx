module;
#include <string>
#include <vector>
#include <filesystem>

export module Brawler.ShaderCompilationParams;
import Brawler.ShaderCompilationFlags;

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

		/// <summary>
		/// This is a bitwise-OR combination of ShaderCompilationFlags instances which
		/// control the shader compilation process. For most shaders, this can be left to
		/// the default value. There are, however, some notable exceptions:
		/// 
		///   - When using bindless SRVs, be sure to add the RESOURCES_MAY_ALIAS flag.
		/// </summary>
		ShaderCompilationFlags CompilationFlags;
	};
}