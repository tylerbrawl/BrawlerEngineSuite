module;
#include <string>
#include <array>
#include <span>
#include <type_traits>

export module Brawler.RootParameterDefinition;
import Brawler.RootParameters;

namespace Brawler
{
	namespace RootParameters
	{
		template <typename T>
			requires std::is_enum_v<T> && requires (T x)
		{
			T::COUNT_OR_ERROR;
		}
		struct RootParameterDefinition
		{
			static_assert(sizeof(T) != sizeof(T), "ERROR: An explicit template specialization of Brawler::RootParameters::RootParameterDefinition was never provided for a particular root parameter enumeration type! (See RootParameterDefinition.ixx.)");
		};

		template <>
		struct RootParameterDefinition<Brawler::RootParameters::BC6HBC7Compression>
		{
			static constexpr std::string_view ROOT_PARAMETER_ENUM_CLASS_NAME{ "BC6HBC7Compression" };

			static constexpr std::array<std::string_view, std::to_underlying(BC6HBC7Compression::COUNT_OR_ERROR)> ROOT_PARAMETER_ENUM_VALUE_STRINGS_ARR{
				"SOURCE_TEXTURE_SRV_TABLE",
				"INPUT_BUFFER_SRV_TABLE",
				"OUTPUT_BUFFER_UAV_TABLE",
				"COMPRESSION_SETTINGS_CBV",
				"MODE_ID_AND_START_BLOCK_NUM_ROOT_CONSTANTS"
			};
		};

		template <>
		struct RootParameterDefinition<Brawler::RootParameters::GenericDownsample>
		{
			static constexpr std::string_view ROOT_PARAMETER_ENUM_CLASS_NAME{ "GenericDownsample" };

			static constexpr std::array<std::string_view, std::to_underlying(GenericDownsample::COUNT_OR_ERROR)> ROOT_PARAMETER_ENUM_VALUE_STRINGS_ARR{
				"TEXTURES_TABLE",
				"MIP_MAP_CONSTANTS"
			};
		};

		template <>
		struct RootParameterDefinition<Brawler::RootParameters::TestRootSignature>
		{
			static constexpr std::string_view ROOT_PARAMETER_ENUM_CLASS_NAME{ "TestRootSignature" };
			
			static constexpr std::array<std::string_view, std::to_underlying(TestRootSignature::COUNT_OR_ERROR)> ROOT_PARAMETER_ENUM_VALUE_STRINGS_ARR{
				"PARAM_0",
				"PARAM_1",
				"PARAM_2",
				"PARAM_3",
				"PARAM_4",
				"PARAM_5",
				"PARAM_6"
			};
		};
	}
}

export namespace Brawler
{
	namespace RootParameters
	{
		template <typename T>
			requires requires (T x)
		{
			RootParameterDefinition<T>::ROOT_PARAMETER_ENUM_CLASS_NAME;
		}
		consteval std::string_view GetEnumClassNameString()
		{
			return RootParameterDefinition<T>::ROOT_PARAMETER_ENUM_CLASS_NAME;
		}

		template <typename T>
			requires requires (T x)
		{
			RootParameterDefinition<T>::ROOT_PARAMETER_ENUM_VALUE_STRINGS_ARR;
			T::COUNT_OR_ERROR;
		}
		consteval auto GetEnumValueStrings()
		{
			return std::span<const std::string_view, std::to_underlying(T::COUNT_OR_ERROR)>{ RootParameterDefinition<T>::ROOT_PARAMETER_ENUM_VALUE_STRINGS_ARR };
		}
	}
}