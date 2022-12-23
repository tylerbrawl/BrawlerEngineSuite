module;
#include <utility>
#include <array>
#include <string>
#include <charconv>
#include <cassert>
#include <ranges>

export module Brawler.ShaderProfileDefinition;
import Brawler.ConvenienceTypes;
import Brawler.ShaderProfileID;
import Brawler.RootSignatureID;
import Brawler.PSOID;
import Brawler.CommandSignatureID;

namespace Brawler
{
	namespace ShaderProfiles
	{
		template <ShaderProfileID ProfileID>
		struct ShaderProfileDefinition
		{
			static_assert(sizeof(ProfileID) != sizeof(ProfileID), "ERROR: An explicit template specialization of Brawler::ShaderProfileDefinition was not provided for a particular ShaderProfileID! (See ShaderProfileDefinition.ixx.)");
		};

		template <>
		struct ShaderProfileDefinition<ShaderProfileID::BRAWLER_ENGINE>
		{
		private:
			static constexpr Brawler::EnumSequence<Brawler::CommandSignatureID,
				Brawler::CommandSignatureID::DEFERRED_GEOMETRY_RASTER
			> RELEVANT_COMMAND_SIGNATURE_IDS_SEQUENCE{};

		public:
			static constexpr Brawler::EnumSequence<Brawler::PSOID,
				Brawler::PSOID::DEFERRED_GEOMETRY_RASTER,
				Brawler::PSOID::MODEL_INSTANCE_FRUSTUM_CULL,
				Brawler::PSOID::DEFERRED_OPAQUE_SHADE
			> RELEVANT_PSO_IDS_ARR{};

			static constexpr Brawler::EnumSequence<Brawler::RootSignatureID,
				Brawler::RootSignatureID::DEFERRED_GEOMETRY_RASTER,
				Brawler::RootSignatureID::MODEL_INSTANCE_FRUSTUM_CULL,
				Brawler::RootSignatureID::DEFERRED_OPAQUE_SHADE
			> RELEVANT_ROOT_SIGNATURE_IDS_ARR{};

			static constexpr auto RELEVANT_COMMAND_SIGNATURE_IDS_ARR{ [] <std::underlying_type_t<Brawler::CommandSignatureID>... CSIdentifiers>(std::integer_sequence<std::underlying_type_t<Brawler::CommandSignatureID>, CSIdentifiers...>)
			{
				return std::array<Brawler::CommandSignatureID, sizeof...(CSIdentifiers)>{ static_cast<Brawler::CommandSignatureID>(CSIdentifiers)... };
			}(RELEVANT_COMMAND_SIGNATURE_IDS_SEQUENCE) };

			static constexpr std::string_view FRIENDLY_PROFILE_NAME_STR{ "Brawler Engine" };
			static constexpr std::string_view CMD_LINE_SELECTION_STR{ "engine" };
			static constexpr std::string_view CMD_LINE_DESCRIPTION_STR{ "This shader profile is meant to be used with the Brawler Engine." };
		};

		template <>
		struct ShaderProfileDefinition<ShaderProfileID::MODEL_EXPORTER>
		{
			static constexpr Brawler::EnumSequence<Brawler::PSOID,
				Brawler::PSOID::BC7_TRY_MODE_456,
				Brawler::PSOID::BC7_TRY_MODE_137,
				Brawler::PSOID::BC7_TRY_MODE_02,
				Brawler::PSOID::BC7_ENCODE_BLOCK,
				Brawler::PSOID::GENERIC_DOWNSAMPLE,
				Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR_SRGB,
				Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC_SRGB
			> RELEVANT_PSO_IDS_ARR{};

			static constexpr Brawler::EnumSequence<Brawler::RootSignatureID,
				Brawler::RootSignatureID::BC6H_BC7_COMPRESSION,
				Brawler::RootSignatureID::GENERIC_DOWNSAMPLE,
				Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_TILING,
				Brawler::RootSignatureID::VIRTUAL_TEXTURE_PAGE_MERGING
			> RELEVANT_ROOT_SIGNATURE_IDS_ARR{};

			static constexpr std::array<Brawler::CommandSignatureID, 0> RELEVANT_COMMAND_SIGNATURE_IDS_ARR{};
			
			static constexpr std::string_view FRIENDLY_PROFILE_NAME_STR{ "Brawler Model Exporter" };
			static constexpr std::string_view CMD_LINE_SELECTION_STR{ "model" };
			static constexpr std::string_view CMD_LINE_DESCRIPTION_STR{ "This shader profile is meant to be used with the Brawler Model Exporter." };
		};

		template <>
		struct ShaderProfileDefinition<ShaderProfileID::TEST_SHADER_PROFILE>
		{
			static constexpr Brawler::EnumSequence<Brawler::PSOID,
				Brawler::PSOID::TEST_PSO
			> RELEVANT_PSO_IDS_ARR{};

			static constexpr Brawler::EnumSequence<Brawler::RootSignatureID,
				Brawler::RootSignatureID::TEST_ROOT_SIGNATURE
			> RELEVANT_ROOT_SIGNATURE_IDS_ARR{};

			static constexpr std::array<Brawler::CommandSignatureID, 0> RELEVANT_COMMAND_SIGNATURE_IDS_ARR{};

			static constexpr std::string_view FRIENDLY_PROFILE_NAME_STR{ "Debug Shader Profile" };
			static constexpr std::string_view CMD_LINE_SELECTION_STR{ "test" };
			static constexpr std::string_view CMD_LINE_DESCRIPTION_STR{ "This shader profile is meant for debugging. It is not meant to be used to serialize PSOs and root signatures for any actual application." };
		};

		template <ShaderProfileID ProfileID>
		concept IsRecognizedShaderProfile = requires (ShaderProfileID x)
		{
			ShaderProfileDefinition<ProfileID>::RELEVANT_PSO_IDS_ARR;
			ShaderProfileDefinition<ProfileID>::RELEVANT_ROOT_SIGNATURE_IDS_ARR;
			ShaderProfileDefinition<ProfileID>::RELEVANT_COMMAND_SIGNATURE_IDS_ARR;
			ShaderProfileDefinition<ProfileID>::FRIENDLY_PROFILE_NAME_STR;
			ShaderProfileDefinition<ProfileID>::CMD_LINE_SELECTION_STR;
			ShaderProfileDefinition<ProfileID>::CMD_LINE_DESCRIPTION_STR;
		};
	}
}

namespace Brawler
{
	namespace ShaderProfiles
	{
		template <std::integral T>
		constexpr std::string ConvertIntegerToString(const T value)
		{
			// std::to_chars is a non-allocating function, so we want to allocate for a worst-case
			// scenario and then shrink to fit. The largest number of characters needed to express
			// an integral value is 20. (std::numeric_limits<std::uint64_t>::max() has 20 digits,
			// while std::numeric_limits<std::int64_t>::min() has 19 digits and one (1) sign character.)
			constexpr std::size_t MAX_CHARS_FOR_VALUE_STR = 20;

			std::string integerStr{};
			integerStr.resize(MAX_CHARS_FOR_VALUE_STR);

			const std::to_chars_result toCharsResult{ std::to_chars(integerStr.data(), (integerStr.data() + integerStr.size()), value) };
			assert(toCharsResult.ec == std::errc{});

			const std::ptrdiff_t numCharsWritten = (toCharsResult.ptr - integerStr.data());
			integerStr.resize(numCharsWritten);

			return integerStr;
		}

		template <ShaderProfileID ProfileID>
		constexpr std::string CreateCompletionDiagnosticsString()
		{
			std::string diagnosticsStr{
R"(
SUCCESS: All files were generated successfully. Remember to add them to the build for this VC++ project!

Shader Profile Build Report:
	- Profile Name: )"
			};

			constexpr std::string_view FRIENDLY_PROFILE_NAME_STR{ ShaderProfileDefinition<ProfileID>::FRIENDLY_PROFILE_NAME_STR };
			diagnosticsStr += FRIENDLY_PROFILE_NAME_STR;

			diagnosticsStr += 
R"(
	- Pipeline State Object (PSO) Count: )";

			constexpr std::size_t PSO_COUNT = ShaderProfileDefinition<ProfileID>::RELEVANT_PSO_IDS_ARR.size();
			diagnosticsStr += ConvertIntegerToString(PSO_COUNT);

			diagnosticsStr +=
R"(
	- Root Signature Count: )";

			constexpr std::size_t ROOT_SIGNATURE_COUNT = ShaderProfileDefinition<ProfileID>::RELEVANT_ROOT_SIGNATURE_IDS_ARR.size();
			diagnosticsStr += ConvertIntegerToString(ROOT_SIGNATURE_COUNT);

			diagnosticsStr +=
R"(
	- Command Signature Count: )";

			constexpr std::size_t COMMAND_SIGNATURE_COUNT = ShaderProfileDefinition<ProfileID>::RELEVANT_COMMAND_SIGNATURE_IDS_ARR.size();
			diagnosticsStr += ConvertIntegerToString(COMMAND_SIGNATURE_COUNT);

			return diagnosticsStr;
		}

		template <ShaderProfileID ProfileID>
		static constexpr auto COMPLETION_DIAGNOSTICS_CHARACTER_ARR{ []()
		{
			constexpr std::size_t DIAGNOSTICS_STR_SIZE = CreateCompletionDiagnosticsString<ProfileID>().size();

			// Add one for the NUL-terminating character.
			std::array<char, (DIAGNOSTICS_STR_SIZE + 1)> diagnosticsCharArr{};
			const std::string diagnosticsStr{ CreateCompletionDiagnosticsString<ProfileID>() };

			for (auto [destChar, srcChar] : std::views::zip(diagnosticsCharArr, diagnosticsStr))
				destChar = srcChar;

			return diagnosticsCharArr;
		}() };

		template <ShaderProfileID ProfileID>
		static constexpr std::string_view COMPLETION_DIAGNOSTICS_STR{ COMPLETION_DIAGNOSTICS_CHARACTER_ARR<ProfileID>.data() };
	}
}

export namespace Brawler
{
	namespace ShaderProfiles
	{
		template <ShaderProfileID ProfileID>
			requires IsRecognizedShaderProfile<ProfileID>
		consteval auto GetPSOIdentifiers()
		{
			return ShaderProfileDefinition<ProfileID>::RELEVANT_PSO_IDS_ARR;
		}

		template <ShaderProfileID ProfileID>
			requires IsRecognizedShaderProfile<ProfileID>
		consteval auto GetRootSignatureIdentifiers()
		{
			return ShaderProfileDefinition<ProfileID>::RELEVANT_ROOT_SIGNATURE_IDS_ARR;
		}

		template <ShaderProfileID ProfileID>
			requires IsRecognizedShaderProfile<ProfileID>
		consteval auto GetCommandSignatureIdentifiers()
		{
			return ShaderProfileDefinition<ProfileID>::RELEVANT_COMMAND_SIGNATURE_IDS_ARR;
		}

		template <ShaderProfileID ProfileID>
			requires IsRecognizedShaderProfile<ProfileID>
		consteval std::string_view GetCommandLineSelectionString()
		{
			return ShaderProfileDefinition<ProfileID>::CMD_LINE_SELECTION_STR;
		}

		template <ShaderProfileID ProfileID>
			requires IsRecognizedShaderProfile<ProfileID>
		consteval std::string_view GetCommandLineDescriptionString()
		{
			return ShaderProfileDefinition<ProfileID>::CMD_LINE_DESCRIPTION_STR;
		}

		template <ShaderProfileID ProfileID>
			requires IsRecognizedShaderProfile<ProfileID>
		consteval std::string_view GetCompletionDiagnosticsString()
		{
			return COMPLETION_DIAGNOSTICS_STR<ProfileID>;
		}
	}
	
}