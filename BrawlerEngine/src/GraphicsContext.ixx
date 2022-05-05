module;
#include <iostream>
#include <array>
#include "DxDef.h"

export module Brawler.GraphicsContext;
import Brawler.I_RenderContext;
import Brawler.PipelineEnums;
import Brawler.GraphicsRootParameterBinder;
import Util.Engine;
import Brawler.PSOManager;

export namespace Brawler
{
	class GraphicsContext final : public I_RenderContext
	{
	public:
		GraphicsContext();

		template <Brawler::PSOID PipelineStateObjectID>
		GraphicsRootParameterBinder<PipelineStateObjectID> SetPipelineStateObject();
	};
}

// ----------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::PSOID PipelineStateObjectID>
	GraphicsRootParameterBinder<PipelineStateObjectID> GraphicsContext::SetPipelineStateObject()
	{
		Brawler::D3D12GraphicsCommandList& cmdList{ GetCommandList() };

		ID3D12PipelineState& psoState{ Util::Engine::GetPSOManager().GetPipelineStateObject(PipelineStateObjectID) };
		cmdList.SetPipelineState(&psoState);

		ID3D12RootSignature& rootSignature{ Util::Engine::GetPSOManager().GetRootSignature(PipelineStateObjectID) };
		cmdList.SetGraphicsRootSignature(&rootSignature);

		return GraphicsRootParameterBinder<PipelineStateObjectID>{ cmdList };
	}
}