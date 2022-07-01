module;
#include <utility>
#include <string>

export module Brawler.ShaderProfileDefinition;
import Brawler.ConvenienceTypes;
import Brawler.ShaderProfileID;
import Brawler.RootSignatureID;
import Brawler.PSOID;

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

			static constexpr std::string_view CMD_LINE_SELECTION_STR{ "test" };
			static constexpr std::string_view CMD_LINE_DESCRIPTION_STR{ "This shader profile is meant for debugging. It is not meant to be used to serialize PSOs and root signatures for any actual application." };
		};

		template <ShaderProfileID ProfileID>
		concept IsRecognizedShaderProfile = requires (ShaderProfileID x)
		{
			ShaderProfileDefinition<ProfileID>::RELEVANT_PSO_IDS_ARR;
			ShaderProfileDefinition<ProfileID>::RELEVANT_ROOT_SIGNATURE_IDS_ARR;
			ShaderProfileDefinition<ProfileID>::CMD_LINE_SELECTION_STR;
			ShaderProfileDefinition<ProfileID>::CMD_LINE_DESCRIPTION_STR;
		};
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
	}
	
}