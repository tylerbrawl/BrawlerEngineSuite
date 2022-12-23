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
	struct RootSignatureDefinition<RootSignatureID::DEFERRED_GEOMETRY_RASTER>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "DEFERRED_GEOMETRY_RASTER" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = (
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |

			// NOTE: As of writing this, the pixel shaders using the DEFERRED_GEOMETRY_RASTER root
			// signature do not access the root parameters, and instead only use the static samplers.
			// The D3D12 specifications are unclear as to whether 
			// D3D12_ROOT_SIGNATURE_FLAG_DENY_*_SHADER_ROOT_ACCESS affect visibility of static samplers,
			// so we err on the side of caution and allow pixel shaders to access the root signature.

			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
		);
		using RootParamType = Brawler::RootParameters::DeferredGeometryRaster;
	};

	template <>
	struct RootSignatureDefinition<RootSignatureID::MODEL_INSTANCE_FRUSTUM_CULL>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "MODEL_INSTANCE_FRUSTUM_CULL" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
		using RootParamType = Brawler::RootParameters::ModelInstanceFrustumCull;
	};

	template <>
	struct RootSignatureDefinition<RootSignatureID::DEFERRED_OPAQUE_SHADE>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "DEFERRED_OPAQUE_SHADE" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
		using RootParamType = Brawler::RootParameters::DeferredOpaqueShade;
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
	struct RootSignatureDefinition<RootSignatureID::VIRTUAL_TEXTURE_PAGE_TILING>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "VIRTUAL_TEXTURE_PAGE_TILING" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
		using RootParamType = Brawler::RootParameters::VirtualTexturePageTiling;
	};

	template <>
	struct RootSignatureDefinition<RootSignatureID::VIRTUAL_TEXTURE_PAGE_MERGING>
	{
		static constexpr std::string_view ROOT_SIGNATURE_ID_STRING{ "VIRTUAL_TEXTURE_PAGE_MERGING" };
		static constexpr D3D12_ROOT_SIGNATURE_FLAGS ROOT_SIGNATURE_FLAGS = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_NONE;
		using RootParamType = Brawler::RootParameters::VirtualTexturePageMerging;
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