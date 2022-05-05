module;
#include <string>
#include "DxDef.h"

export module Brawler.PSOField;

export namespace Brawler
{
	template <typename PSOSubObjectType>
	struct PSOField
	{
		static_assert(sizeof(PSOSubObjectType) != sizeof(PSOSubObjectType), "ERROR: An explicit template specialization of Brawler::PSOField was never provided for this particular PSO sub-object type! (See PSOField.ixx.)");
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE" };
		static constexpr std::string_view FIELD_NAME{ "pRootSignature" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_VS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_VS" };
		static constexpr std::string_view FIELD_NAME{ "VS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_PS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_PS" };
		static constexpr std::string_view FIELD_NAME{ "PS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_DS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_DS" };
		static constexpr std::string_view FIELD_NAME{ "DS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_HS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_HS" };
		static constexpr std::string_view FIELD_NAME{ "HS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_GS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_GS" };
		static constexpr std::string_view FIELD_NAME{ "GS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_CS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_CS" };
		static constexpr std::string_view FIELD_NAME{ "CS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT" };
		static constexpr std::string_view FIELD_NAME{ "StreamOutput" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC" };
		static constexpr std::string_view FIELD_NAME{ "BlendDesc" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK" };
		static constexpr std::string_view FIELD_NAME{ "SampleMask" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER" };
		static constexpr std::string_view FIELD_NAME{ "RasterizerState" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1" };
		static constexpr std::string_view FIELD_NAME{ "DepthStencilState" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT" };
		static constexpr std::string_view FIELD_NAME{ "InputLayout" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE" };
		static constexpr std::string_view FIELD_NAME{ "IBStripCutValue" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY" };
		static constexpr std::string_view FIELD_NAME{ "PrimitiveTopologyType" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS" };
		static constexpr std::string_view FIELD_NAME{ "RenderTargetFormats" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT" };
		static constexpr std::string_view FIELD_NAME{ "DepthStencilFormat" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC" };
		static constexpr std::string_view FIELD_NAME{ "SampleDesc" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK" };
		static constexpr std::string_view FIELD_NAME{ "NodeMask" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO" };
		static constexpr std::string_view FIELD_NAME{ "CachedPSO" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_FLAGS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_FLAGS" };
		static constexpr std::string_view FIELD_NAME{ "Flags" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING" };
		static constexpr std::string_view FIELD_NAME{ "ViewInstancingDesc" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_AS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_AS" };
		static constexpr std::string_view FIELD_NAME{ "AS" };
	};

	template <>
	struct PSOField<CD3DX12_PIPELINE_STATE_STREAM_MS>
	{
		static constexpr std::string_view FIELD_TYPE{ "CD3DX12_PIPELINE_STATE_STREAM_MS" };
		static constexpr std::string_view FIELD_NAME{ "MS" };
	};
}