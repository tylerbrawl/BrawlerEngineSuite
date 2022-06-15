module;
#include <string>
#include "DxDef.h"

export module Brawler.RootSignatureDefinition;
import Brawler.RootSignatureID;
import Brawler.RootParameters;

namespace Brawler
{
	template <RootSignatureID RSIdentifier>
	struct RootSignatureDefinition
	{
		static_assert(sizeof(RSIdentifier) != sizeof(RSIdentifier), "ERROR: An explicit template specialization of Brawler::RootSignatureDefinition was never provided for a particular RootSignatureID! (See RootSignatureDefinition.ixx.)");
	};

	template <>
	struct RootSignatureDefinition<RootSignatureID::BC6H_BC7_COMPRESSION>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "BC6H_BC7_COMPRESSION" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
		using RootParamType = Brawler::RootParameters::BC6HBC7Compression;
	};

	template <>
	struct RootSignatureDefinition<RootSignatureID::GENERIC_DOWNSAMPLE>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "GENERIC_DOWNSAMPLE" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
		using RootParamType = Brawler::RootParameters::GenericDownsample;
	};

	template <>
	struct RootSignatureDefinition<RootSignatureID::TEST_ROOT_SIGNATURE>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "TEST_ROOT_SIGNATURE" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
		using RootParamType = Brawler::RootParameters::TestRootSignature;
	};
}

export namespace Brawler
{
	template <RootSignatureID RSIdentifier>
	consteval std::string_view GetRootSignatureIDString()
	{
		return RootSignatureDefinition<RSIdentifier>::ROOT_SIGNATURE_ID_STRING;
	}

	template <RootSignatureID RSIdentifier>
	consteval D3D12_ROOT_SIGNATURE_FLAGS GetRootSignatureFlags()
	{
		return RootSignatureDefinition<RSIdentifier>::ROOT_SIGNATURE_FLAGS;
	}

	template <RootSignatureID RSIdentifier>
	using RootParamType = RootSignatureDefinition<RSIdentifier>::RootParamType;
}