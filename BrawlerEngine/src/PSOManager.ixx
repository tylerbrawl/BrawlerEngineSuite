module;
#include <array>
#include "DxDef.h"

export module Brawler.PSOManager;
import Brawler.PipelineEnums;
import Brawler.PSOData;

export namespace Brawler
{
	class PSOManager
	{
	public:
		PSOManager();

		PSOManager(const PSOManager& rhs) = delete;
		PSOManager& operator=(const PSOManager& rhs) = delete;

		PSOManager(PSOManager&& rhs) noexcept = default;
		PSOManager& operator=(PSOManager&& rhs) noexcept = default;

		void Initialize();

		/// <summary>
		/// Retrieves the ID3D12PipelineState which corresponds to the specified PSOID.
		/// NOTE: This function does *NOT* modify the PSOManager instance. It is not marked
		/// const because ID3D12GraphicsCommandList::SetPipelineState() does not take a
		/// const ID3D12PipelineState*.
		/// </summary>
		/// <returns>
		/// Returns an ID3D12PipelineState& corresponding to the PSO which represents the pipeline
		/// state object corrsponding to PSOID.
		/// </returns>
		ID3D12PipelineState& GetPipelineStateObject(const Brawler::PSOID psoID);

		/// <summary>
		/// Retrieves the ID3D12RootSignature which corresponds to the root signature used
		/// by the PSO specified by PSOID.
		/// NOTE: This function does *NOT* modify the PSOManager instance. It is not marked
		/// const because ID3D12GraphicsCommandList::Set*RootSignature() does not take a
		/// const ID3D12RootSignature*.
		/// </summary>
		/// <returns>
		/// Returns an ID3D12RootSignature& corresponding to the root signature used by the PSO
		/// specified by PSOID.
		/// </returns>
		ID3D12RootSignature& GetRootSignature(const Brawler::PSOID psoID);

	private:
		std::array<PSOData, std::to_underlying(Brawler::PSOID::COUNT_OR_ERROR)> mPSODataMap;
	};
}