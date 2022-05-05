module;
#include "DxDef.h"

export module Brawler.ComputeContext;
import Brawler.I_RenderContext;
import Brawler.PipelineEnums;
import Brawler.ComputeRootParameterBinder;
import Util.Engine;
import Brawler.PSOManager;

export namespace Brawler
{
	class ComputeContext final : public I_RenderContext
	{
	public:
		ComputeContext();

		template <Brawler::PSOID PipelineStateObjectID>
		ComputeRootParameterBinder<PipelineStateObjectID> SetPipelineStateObject();
	};
}

// ----------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::PSOID PipelineStateObjectID>
	ComputeRootParameterBinder<PipelineStateObjectID> ComputeContext::SetPipelineStateObject()
	{
		Brawler::D3D12GraphicsCommandList& cmdList{ GetCommandList() };

		ID3D12PipelineState& psoState{ Util::Engine::GetPSOManager().GetPipelineStateObject(PipelineStateObjectID) };
		cmdList.SetPipelineState(&psoState);

		ID3D12RootSignature& rootSignature{ Util::Engine::GetPSOManager().GetRootSignature(PipelineStateObjectID) };
		cmdList.SetComputeRootSignature(&rootSignature);

		return ComputeRootParameterBinder<PipelineStateObjectID>{ cmdList };
	}
}