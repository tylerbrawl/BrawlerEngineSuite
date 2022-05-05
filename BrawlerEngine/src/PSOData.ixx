module;
#include "DxDef.h"

export module Brawler.PSOData;

export namespace Brawler
{
	struct PSOData
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> PSO;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	};
}