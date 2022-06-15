module;
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <cassert>
#include <span>
#include <compare>  // Classic MSVC modules jank...
#include <filesystem>
#include "DxDef.h"

export module Brawler.PSOShaderFieldResolver;
import Brawler.I_PSOFieldResolver;
import Brawler.PSOField;
import Brawler.ShaderCompilationParams;
import Util.Threading;
import Brawler.AppParams;
import Util.General;
import Brawler.ThreadLocalResources;
import Util.FileWrite;
import Brawler.CondensedByteArrayInfo;
import Brawler.FileStrings;

namespace Brawler
{
	namespace IMPL
	{
		template <typename PSOSubObjectType>
		struct ShaderInfo
		{
			static_assert(sizeof(PSOSubObjectType) != sizeof(PSOSubObjectType), "ERROR: An explicit template specialization of Brawler::IMPL::ShaderInfo was never provided for a given shader PSO sub-object stream type!");
		};

		template <>
		struct ShaderInfo<CD3DX12_PIPELINE_STATE_STREAM_VS>
		{
			static constexpr std::wstring_view SHADER_TARGET{ L"vs_6_0" };
			static constexpr std::string_view SHADER_BYTECODE_ARRAY_FIELD_NAME{ "VERTEX_SHADER_BYTECODE" };
			static constexpr std::string_view SHADER_BYTECODE_SIZE_FIELD_NAME{ "VERTEX_SHADER_BYTECODE_SIZE" };
		};

		template <>
		struct ShaderInfo<CD3DX12_PIPELINE_STATE_STREAM_PS>
		{
			static constexpr std::wstring_view SHADER_TARGET{ L"ps_6_0" };
			static constexpr std::string_view SHADER_BYTECODE_ARRAY_FIELD_NAME{ "PIXEL_SHADER_BYTECODE" };
			static constexpr std::string_view SHADER_BYTECODE_SIZE_FIELD_NAME{ "PIXEL_SHADER_BYTECODE_SIZE" };
		};

		template <>
		struct ShaderInfo<CD3DX12_PIPELINE_STATE_STREAM_CS>
		{
			static constexpr std::wstring_view SHADER_TARGET{ L"cs_6_0" };
			static constexpr std::string_view SHADER_BYTECODE_ARRAY_FIELD_NAME{ "COMPUTE_SHADER_BYTECODE" };
			static constexpr std::string_view SHADER_BYTECODE_SIZE_FIELD_NAME{ "COMPUTE_SHADER_BYTECODE_SIZE" };
		};

		template <>
		struct ShaderInfo<CD3DX12_PIPELINE_STATE_STREAM_DS>
		{
			static constexpr std::wstring_view SHADER_TARGET{ L"ds_6_0" };
			static constexpr std::string_view SHADER_BYTECODE_ARRAY_FIELD_NAME{ "DOMAIN_SHADER_BYTECODE" };
			static constexpr std::string_view SHADER_BYTECODE_SIZE_FIELD_NAME{ "DOMAIN_SHADER_BYTECODE_SIZE" };
		};

		template <>
		struct ShaderInfo<CD3DX12_PIPELINE_STATE_STREAM_GS>
		{
			static constexpr std::wstring_view SHADER_TARGET{ L"gs_6_0" };
			static constexpr std::string_view SHADER_BYTECODE_ARRAY_FIELD_NAME{ "GEOMETRY_SHADER_BYTECODE" };
			static constexpr std::string_view SHADER_BYTECODE_SIZE_FIELD_NAME{ "GEOMETRY_SHADER_BYTECODE_SIZE" };
		};

		template <>
		struct ShaderInfo<CD3DX12_PIPELINE_STATE_STREAM_HS>
		{
			static constexpr std::wstring_view SHADER_TARGET{ L"hs_6_0" };
			static constexpr std::string_view SHADER_BYTECODE_ARRAY_FIELD_NAME{ "HULL_SHADER_BYTECODE" };
			static constexpr std::string_view SHADER_BYTECODE_SIZE_FIELD_NAME{ "HULL_SHADER_BYTECODE_SIZE" };
		};

		template <typename PSOSubObjectType>
		concept IsRecognizedShaderType = requires (PSOSubObjectType x)
		{
			Brawler::PSOField<PSOSubObjectType>::FIELD_NAME;
			ShaderInfo<PSOSubObjectType>::SHADER_TARGET;
		};

		template <typename PSOSubObjectType>
			requires IsRecognizedShaderType<PSOSubObjectType>
		struct HLSLCompileArgumentsInfo
		{
			static constexpr std::array<std::wstring_view, 12> DEBUG_HLSL_COMPILE_ARGUMENTS_ARR{
				// Enable strict mode (whatever that means...).
				L"-Ges",

				// Use HLSL 2021.
				L"-HV", L"2021",

				// Disable optimizations.
				L"-Od",

				// Set the shader target profile.
				L"-T", ShaderInfo<PSOSubObjectType>::SHADER_TARGET,

				// Treat warnings as errors.
				L"-WX",

				// Enable debug information.
				L"-Zi",

				// Pack matrices in row-major order.
				L"-Zpr",

				// Embed the PDB within the shader container.
				L"-Qembed_debug",

				// Add a preprocessor macro to announce that we are building for Debug mode.
				L"-D", L"_DEBUG=1"
			};

			static constexpr std::array<std::wstring_view, 11> RELEASE_HLSL_COMPILE_ARGUMENTS_ARR{
				// Enable aggressive flattening. This enables optimizations by allowing the
				// hardware to assume that all resources which will be accessed by the shader
				// are, in fact, actually bound as a root parameter.
				L"-all-resources-bound",

				// Enable strict mode (whatever that means...).
				L"-Ges",

				// Use HLSL 2021.
				L"-HV", L"2021",

				// Set the shader target profile.
				L"-T", ShaderInfo<PSOSubObjectType>::SHADER_TARGET,

				// Treat warnings as errors.
				L"-WX",

				// Pack matrices in row-major order.
				L"-Zpr",

				// Use the highest optimization level.
				L"-O3",

				// Strip debug information from the shader bytecode.
				L"-Qstrip_debug",

				// Strip reflection data from the shader bytecode.
				L"-Qstrip_reflect"
			};
		};

		template <typename PSOSubObjectType>
			requires IsRecognizedShaderType<PSOSubObjectType>
		std::string GetRuntimePSOResolutionString()
		{
			/*
			psoDesc.[PSOField<PSOSubObjectType>::FIELD_NAME].pShaderBytecode = reinterpret_cast<const void*>([ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_ARRAY_FIELD_NAME].data());
			psoDesc.[PSOField<PSOSubObjectType>::FIELD_NAME].BytecodeLength = [ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_SIZE_FIELD_NAME];
			*/

			std::string resolutionStr{ "\t\t\t\tD3D12_SHADER_BYTECODE& "};
			resolutionStr += PSOField<PSOSubObjectType>::FIELD_NAME;
			resolutionStr += "{ static_cast<D3D12_SHADER_BYTECODE&>(psoDesc.";
			resolutionStr += PSOField<PSOSubObjectType>::FIELD_NAME;
			resolutionStr += ") };\n\t\t\t\t";

			resolutionStr += PSOField<PSOSubObjectType>::FIELD_NAME;
			resolutionStr += ".pShaderBytecode = reinterpret_cast<const void*>(";
			resolutionStr += ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_ARRAY_FIELD_NAME;
			resolutionStr += ".data());\n\t\t\t\t";

			resolutionStr += PSOField<PSOSubObjectType>::FIELD_NAME;
			resolutionStr += ".BytecodeLength = ";
			resolutionStr += ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_SIZE_FIELD_NAME;
			resolutionStr += ";\n";

			return resolutionStr;
		}
	}
}

export namespace Brawler
{
	template <typename PSOSubObjectType>
		requires IMPL::IsRecognizedShaderType<PSOSubObjectType>
	class PSOShaderFieldResolver : public I_PSOFieldResolver
	{
	public:
		explicit PSOShaderFieldResolver(const ShaderCompilationParams& params);

		PSOShaderFieldResolver(const PSOShaderFieldResolver& rhs) = delete;
		PSOShaderFieldResolver& operator=(const PSOShaderFieldResolver& rhs) = delete;

		PSOShaderFieldResolver(PSOShaderFieldResolver&& rhs) noexcept = default;
		PSOShaderFieldResolver& operator=(PSOShaderFieldResolver&& rhs) noexcept = default;

		FileWriterNode CreatePSODefinitionFileWriterNode() const override;
		FileWriterNode CreateRuntimePSOResolutionFileWriterNode() const override;

	private:
		void CompileShader(const ShaderCompilationParams& params);

	private:
		Microsoft::WRL::ComPtr<IDxcBlob> mDebugShaderBlob;
		Microsoft::WRL::ComPtr<IDxcBlob> mReleaseShaderBlob;
	};
}

// ---------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename PSOSubObjectType>
		requires IMPL::IsRecognizedShaderType<PSOSubObjectType>
	PSOShaderFieldResolver<PSOSubObjectType>::PSOShaderFieldResolver(const ShaderCompilationParams& params) :
		mDebugShaderBlob(nullptr),
		mReleaseShaderBlob(nullptr)
	{
		CompileShader(params);
	}

	template <typename PSOSubObjectType>
		requires IMPL::IsRecognizedShaderType<PSOSubObjectType>
	FileWriterNode PSOShaderFieldResolver<PSOSubObjectType>::CreatePSODefinitionFileWriterNode() const
	{
		// The idea is that we want to write out an array of bytes; this array of bytes
		// represents the shader bytecode. However, to prevent excessive code bloat (as
		// well as for security), we don't want to include the bytecode for Debug shaders
		// in Release builds and vice versa.
		// 
		// Unfortunately, the MSVC (specifically, CL.exe) crashes with a stack overflow
		// error when creating an array of bytes with a sufficiently long initializer list.
		// (This only appears to be a problem with C++20 modules; it seems to work just fine
		// with standard .cpp files.) To work around this, we will instead pack the individual
		// bytes into 64-bit unsigned integers and specify as a separate field how large the
		// data actually is.
		// 
		// To be perfectly clear, the problem isn't with the size of the std::array, but with
		// the size of the initializer list. Creating a static constexpr 
		// std::array<std::uint8_t, 500000> with a default constructor works fine, but initializing
		// it with 500,000 unique byte values does not. This is why we aim to "pack" the
		// bytes into 64-bit integers.
		//
		// We don't use std::stringstream because it is implemented with a global
		// critical section in the MSVC STL.
		std::string bytecodeStr{};

		// Add the Debug shader bytecode, but only in Debug builds.
		{
			/*
			#ifdef _DEBUG
			static constexpr std::array<std::uint64_t, [64-Bit Integer Initializer List Length]> [IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_ARRAY_FIELD_NAME]{[Incomprehensible Byte List]};
			static constexpr std::size_t [IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_SIZE_FIELD_NAME] = [Shader Size in Bytes];
			*/
			const LPVOID blobData = mDebugShaderBlob->GetBufferPointer();
			const std::size_t blobSize = mDebugShaderBlob->GetBufferSize();

			Brawler::CondensedByteArrayInfo debugShaderBytecodeArrayInfo{ Util::FileWrite::CreateSTDUInt64ArrayContentsStringFromBuffer(std::span<const std::uint8_t>{ reinterpret_cast<const std::uint8_t*>(mDebugShaderBlob->GetBufferPointer()), mDebugShaderBlob->GetBufferSize() }) };

			bytecodeStr += "#ifdef _DEBUG\n\t\t\tstatic constexpr std::array<std::uint64_t, ";
			bytecodeStr += std::to_string(debugShaderBytecodeArrayInfo.InitializerListLengthInElements);
			bytecodeStr += "> ";
			bytecodeStr += IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_ARRAY_FIELD_NAME;
			bytecodeStr += "{";
			bytecodeStr += std::move(debugShaderBytecodeArrayInfo.UInt64ByteArrayInitializerListContents);
			bytecodeStr += "};\n\t\t\tstatic constexpr std::size_t ";
			bytecodeStr += IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_SIZE_FIELD_NAME;
			bytecodeStr += " = ";
			bytecodeStr += std::to_string(debugShaderBytecodeArrayInfo.ActualDataSizeInBytes);
			bytecodeStr += ";\n";
		}

		// Add the Release shader bytecode, but only in Release builds.
		{
			/*
			#else
			static constexpr std::array<std::uint64_t, [64-Bit Integer Initializer List Length]> [IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_ARRAY_FIELD_NAME]{[Incomprehensible Byte List]};
			static constexpr std::size_t [IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_SIZE_FIELD_NAME] = [Shader Size in Bytes];
			#endif
			*/
			Brawler::CondensedByteArrayInfo releaseShaderBytecodeArrayInfo{ Util::FileWrite::CreateSTDUInt64ArrayContentsStringFromBuffer(std::span<const std::uint8_t>{ reinterpret_cast<const std::uint8_t*>(mReleaseShaderBlob->GetBufferPointer()), mReleaseShaderBlob->GetBufferSize() }) };

			bytecodeStr += "#else\n\t\t\tstatic constexpr std::array<std::uint64_t, ";
			bytecodeStr += std::to_string(releaseShaderBytecodeArrayInfo.InitializerListLengthInElements);
			bytecodeStr += "> ";
			bytecodeStr += IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_ARRAY_FIELD_NAME;
			bytecodeStr += "{";
			bytecodeStr += std::move(releaseShaderBytecodeArrayInfo.UInt64ByteArrayInitializerListContents);
			bytecodeStr += "};\n\t\t\tstatic constexpr std::size_t ";
			bytecodeStr += IMPL::ShaderInfo<PSOSubObjectType>::SHADER_BYTECODE_SIZE_FIELD_NAME;
			bytecodeStr += " = ";
			bytecodeStr += std::to_string(releaseShaderBytecodeArrayInfo.ActualDataSizeInBytes);
			bytecodeStr += ";\n#endif\n\n";
		}

		FileWriterNode fileWriterNode{};
		fileWriterNode.SetOutputText(std::move(bytecodeStr));

		return fileWriterNode;
	}

	template <typename PSOSubObjectType>
		requires IMPL::IsRecognizedShaderType<PSOSubObjectType>
	FileWriterNode PSOShaderFieldResolver<PSOSubObjectType>::CreateRuntimePSOResolutionFileWriterNode() const
	{
		// The goal here is to assign the shader bytecode at runtime to the PSO description.
		// Although it *MIGHT* be possible to do this at compile time, the implementation of
		// such a feature would be extraordinarily complex.

		FileWriterNode fileWriterNode{};
		fileWriterNode.SetOutputText(IMPL::GetRuntimePSOResolutionString<PSOSubObjectType>());

		return fileWriterNode;
	}

	template <typename PSOSubObjectType>
		requires IMPL::IsRecognizedShaderType<PSOSubObjectType>
	void PSOShaderFieldResolver<PSOSubObjectType>::CompileShader(const ShaderCompilationParams& params)
	{
		enum class ShaderCompilationMode
		{
			DEBUG,
			RELEASE
		};

		Brawler::ThreadLocalResources& threadLocalResources{ Util::Threading::GetThreadLocalResources() };
		
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> hlslIncludeHandler{};
		CheckHRESULT(threadLocalResources.DXCUtils->CreateDefaultIncludeHandler(&hlslIncludeHandler));

		// Read the HLSL file.
		std::error_code errorCode{};
		const std::filesystem::path absoluteShaderPath{ std::filesystem::absolute(Util::General::GetLaunchParameters().RootSourceDirectory / params.FilePath, errorCode) };

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::string{"ERROR: The attempt to get the absolute path of a shader failed with the following error: "} + errorCode.message() };

		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob{};
		CheckHRESULT(threadLocalResources.DXCUtils->LoadFile(absoluteShaderPath.c_str(), nullptr, &sourceBlob));

		const DxcBuffer sourceBuffer{
			.Ptr = sourceBlob->GetBufferPointer(),
			.Size = sourceBlob->GetBufferSize(),
			.Encoding = DXC_CP_UTF8
		};

		auto compilationLambda = [&absoluteShaderPath]<ShaderCompilationMode CompileMode>(const ShaderCompilationParams& params, const DxcBuffer& srcBuffer, IDxcIncludeHandler& includeHandler) -> Microsoft::WRL::ComPtr<IDxcBlob>
		{
			std::vector<const wchar_t*> compileParamsCStrArr{};

			if constexpr (CompileMode == ShaderCompilationMode::DEBUG)
			{
				compileParamsCStrArr.reserve(IMPL::HLSLCompileArgumentsInfo<PSOSubObjectType>::DEBUG_HLSL_COMPILE_ARGUMENTS_ARR.size() + (2 * params.MacroDefinitionArr.size()) + 3);

				// We can optionally specify the shader source file name for error reporting and
				// PIX shader source viewing. 
				compileParamsCStrArr.push_back(absoluteShaderPath.c_str());

				// Specify the entry point of the shader.
				compileParamsCStrArr.push_back(L"-E");
				compileParamsCStrArr.push_back(params.EntryPoint.data());

				// Add the rest of the standard compilation parameters.
				for (const auto& param : IMPL::HLSLCompileArgumentsInfo<PSOSubObjectType>::DEBUG_HLSL_COMPILE_ARGUMENTS_ARR)
					compileParamsCStrArr.push_back(param.data());
			}
			else
			{
				compileParamsCStrArr.reserve(IMPL::HLSLCompileArgumentsInfo<PSOSubObjectType>::RELEASE_HLSL_COMPILE_ARGUMENTS_ARR.size() + (2 * params.MacroDefinitionArr.size()) + 3);

				// We can optionally specify the shader source file name for error reporting and
				// PIX shader source viewing.
				compileParamsCStrArr.push_back(absoluteShaderPath.c_str());

				// Specify the entry point of the shader.
				compileParamsCStrArr.push_back(L"-E");
				compileParamsCStrArr.push_back(params.EntryPoint.data());

				// Add the rest of the standard compilation parameters.
				for (const auto& param : IMPL::HLSLCompileArgumentsInfo<PSOSubObjectType>::RELEASE_HLSL_COMPILE_ARGUMENTS_ARR)
					compileParamsCStrArr.push_back(param.data());
			}

			// Add each of the specified compilation parameters.
			constexpr std::wstring_view MACRO_DEFINITION_ARGUMENT{ L"-D" };

			std::vector<std::wstring> macroDefinitionStrArr{};
			macroDefinitionStrArr.reserve(params.MacroDefinitionArr.size());

			{
				for (const auto& macroDefinition : params.MacroDefinitionArr)
				{
					// Add the command line argument.
					compileParamsCStrArr.push_back(MACRO_DEFINITION_ARGUMENT.data());

					// Create the backing storage for the macro definition string.
					macroDefinitionStrArr.push_back(std::wstring{ std::wstring{ macroDefinition.MacroName } + L"=" + std::wstring{ macroDefinition.MacroValue } });

					// Add the macro definition string's pointer.
					compileParamsCStrArr.push_back(macroDefinitionStrArr.back().c_str());
				}
			}

			// Compile the shader.
			Microsoft::WRL::ComPtr<IDxcResult> compileResults{};
			CheckHRESULT(Util::Threading::GetThreadLocalResources().DXCShaderCompiler->Compile(
				&srcBuffer,
				compileParamsCStrArr.data(),
				static_cast<std::uint32_t>(compileParamsCStrArr.size()),
				&includeHandler,
				IID_PPV_ARGS(&compileResults)
			));

			// Check for compilation errors.
			Microsoft::WRL::ComPtr<IDxcBlobUtf8> errorsBlob{};
			CheckHRESULT(compileResults->GetOutput(DXC_OUT_KIND::DXC_OUT_ERRORS, IID_PPV_ARGS(&errorsBlob), nullptr));

			// IDxcCompiler3::Compile should always return an error buffer.
			assert(errorsBlob != nullptr && "ERROR The DXC documentation lied about the creation of error buffers!");

			if (errorsBlob->GetStringLength() > 0) [[unlikely]]
			{
				// The shader failed to compile.
				throw std::runtime_error{ std::string{ "ERROR: The shader file " } + absoluteShaderPath.string() + " failed to compile! The errors are as follows:\n" + errorsBlob->GetStringPointer()};
			}

			// If there weren't any errors, then the shader should have compiled successfully.
#ifdef _DEBUG
			HRESULT compileHR{};
			CheckHRESULT(compileResults->GetStatus(&compileHR));

			assert(SUCCEEDED(compileHR));
#endif // _DEBUG

			// Save the compiled shader.
			Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob{};
			CheckHRESULT(compileResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr));

			assert(shaderBlob != nullptr);

			return shaderBlob;
		};

		mDebugShaderBlob = compilationLambda.operator()<ShaderCompilationMode::DEBUG>(params, sourceBuffer, *(hlslIncludeHandler.Get()));
		mReleaseShaderBlob = compilationLambda.operator()<ShaderCompilationMode::RELEASE>(params, sourceBuffer, *(hlslIncludeHandler.Get()));
	}
}