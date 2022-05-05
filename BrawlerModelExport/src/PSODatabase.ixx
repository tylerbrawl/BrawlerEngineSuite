module;
#include <array>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.PSODatabase;
import Brawler.PSOs.PSOID;

namespace Brawler
{
	class JobGroup;
}

export namespace Brawler
{
	namespace D3D12
	{
		class PSODatabase
		{
		public:
			PSODatabase() = default;

			PSODatabase(const PSODatabase& rhs) = delete;
			PSODatabase& operator=(const PSODatabase& rhs) = delete;

			PSODatabase(PSODatabase&& rhs) noexcept = default;
			PSODatabase& operator=(PSODatabase&& rhs) noexcept = default;

			void Initialize();

			template <Brawler::PSOs::PSOID PSOIdentifier>
				requires (PSOIdentifier != Brawler::PSOs::PSOID::COUNT_OR_ERROR)
			Brawler::D3D12PipelineState& GetPipelineState();

			template <Brawler::PSOs::PSOID PSOIdentifier>
				requires (PSOIdentifier != Brawler::PSOs::PSOID::COUNT_OR_ERROR)
			const Brawler::D3D12PipelineState& GetPipelineState() const;

		private:
			template <Brawler::PSOs::PSOID PSOIdentifer>
			HRESULT CompilePSO();

			template <Brawler::PSOs::PSOID PSOIdentifier>
			void AddPSOCompilationJob(Brawler::JobGroup& psoCompileGroup, std::array<HRESULT, std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR)>& compileResultsArr);

		private:
			std::array<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>, std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR)> mPSOMap;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
			requires (PSOIdentifier != Brawler::PSOs::PSOID::COUNT_OR_ERROR)
		Brawler::D3D12PipelineState& PSODatabase::GetPipelineState()
		{
			assert(mPSOMap[std::to_underlying(PSOIdentifier)] != nullptr && "ERROR: An attempt was made to access a PSO in a PSODatabase before it could be compiled!");
			return *(mPSOMap[std::to_underlying(PSOIdentifier)].Get());
		}

		template <Brawler::PSOs::PSOID PSOIdentifier>
			requires (PSOIdentifier != Brawler::PSOs::PSOID::COUNT_OR_ERROR)
		const Brawler::D3D12PipelineState& PSODatabase::GetPipelineState() const
		{
			assert(mPSOMap[std::to_underlying(PSOIdentifier)] != nullptr && "ERROR: An attempt was made to access a PSO in a PSODatabase before it could be compiled!");
			return *(mPSOMap[std::to_underlying(PSOIdentifier)].Get());
		}
	}
}