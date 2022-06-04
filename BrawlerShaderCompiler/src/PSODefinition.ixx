module;
#include <string>
#include "DxDef.h"

export module Brawler.PSODefinition;
import Brawler.PSOID;
import Brawler.RootSignatureID;

namespace Brawler
{
	namespace IMPL
	{
		struct StandardGraphicsPSOStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_DS DS;
			CD3DX12_PIPELINE_STATE_STREAM_HS HS;
			CD3DX12_PIPELINE_STATE_STREAM_GS GS;
			CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT StreamOutput;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 DepthStencilState;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE IBStripCutValue;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RenderTargetFormats;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DepthStencilFormat;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
			CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO CachedPSO;
			CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
		};

		struct StandardComputePSOStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO CachedPSO;
			CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
		};
	}
}

namespace Brawler
{
	template <PSOID PSOIdentifier>
	struct PSODefinition
	{
		static_assert(sizeof(PSOIdentifier) != sizeof(PSOIdentifier), "ERROR: An explicit template specialization of Brawler::PSODefinition was never provided for this PSOID! (See PSODefinition.ixx.)");
	};

	template <>
	struct PSODefinition<PSOID::BC7_TRY_MODE_456>
	{
		static constexpr std::string_view PSO_ID_STRING{ "BC7_TRY_MODE_456" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::BC6H_BC7_COMPRESSION;
		using PSOStreamType = IMPL::StandardComputePSOStream;
	};

	template <>
	struct PSODefinition<PSOID::BC7_TRY_MODE_137>
	{
		static constexpr std::string_view PSO_ID_STRING{ "BC7_TRY_MODE_137" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::BC6H_BC7_COMPRESSION;
		using PSOStreamType = IMPL::StandardComputePSOStream;
	};

	template <>
	struct PSODefinition<PSOID::BC7_TRY_MODE_02>
	{
		static constexpr std::string_view PSO_ID_STRING{ "BC7_TRY_MODE_02" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::BC6H_BC7_COMPRESSION;
		using PSOStreamType = IMPL::StandardComputePSOStream;
	};

	template <>
	struct PSODefinition<PSOID::BC7_ENCODE_BLOCK>
	{
		static constexpr std::string_view PSO_ID_STRING{ "BC7_ENCODE_BLOCK" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::BC6H_BC7_COMPRESSION;
		using PSOStreamType = IMPL::StandardComputePSOStream;
	};

	template <>
	struct PSODefinition<PSOID::GENERIC_DOWNSAMPLE>
	{
		static constexpr std::string_view PSO_ID_STRING{ "GENERIC_DOWNSAMPLE" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::GENERIC_DOWNSAMPLE;
		using PSOStreamType = IMPL::StandardComputePSOStream;
	};

	template <>
	struct PSODefinition<PSOID::GENERIC_DOWNSAMPLE_SRGB>
	{
		static constexpr std::string_view PSO_ID_STRING{ "GENERIC_DOWNSAMPLE_SRGB" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::GENERIC_DOWNSAMPLE;
		using PSOStreamType = IMPL::StandardComputePSOStream;
	};
	
	template <>
	struct PSODefinition<PSOID::TEST_PSO>
	{
		static constexpr std::string_view PSO_ID_STRING{ "TEST_PSO" };
		static constexpr RootSignatureID ROOT_SIGNATURE_ID = RootSignatureID::TEST_ROOT_SIGNATURE;
		using PSOStreamType = IMPL::StandardGraphicsPSOStream;
	};
}

export namespace Brawler
{
	template <PSOID PSOIdentifier>
	consteval std::string_view GetPSOIDString()
	{
		return PSODefinition<PSOIdentifier>::PSO_ID_STRING;
	}

	template <PSOID PSOIdentifier>
	consteval RootSignatureID GetRootSignatureID()
	{
		return PSODefinition<PSOIdentifier>::ROOT_SIGNATURE_ID;
	}

	template <PSOID PSOIdentifier>
	using PSOStreamType = PSODefinition<PSOIdentifier>::PSOStreamType;
}