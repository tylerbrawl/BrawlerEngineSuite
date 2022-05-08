module;
#include <array>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.PSODatabase;
import Brawler.JobSystem;
import Util.Engine;
import Brawler.PSOs.PSODefinition;

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
		HRESULT PSODatabase::CompilePSO()
		{
			// TODO: Create some way to retrieve cached PSOs. Alternatively (and preferably), we
			// could edit the generated PSODefinition::ExecuteRuntimePSOResolution() function to
			// do this for us.

			auto psoStream{ Brawler::PSOs::CreatePSODescription<PSOIdentifier>() };

			const D3D12_PIPELINE_STATE_STREAM_DESC psoDesc{ 
				.SizeInBytes = sizeof(psoStream),
				.pPipelineStateSubobjectStream = reinterpret_cast<void*>(std::addressof(psoStream))
			};
			return Util::Engine::GetD3D12Device().CreatePipelineState(&psoDesc, IID_PPV_ARGS(&(mPSOMap[std::to_underlying(PSOIdentifier)])));
		}

		template <Brawler::PSOs::PSOID PSOIdentifier>
		void PSODatabase::AddPSOCompilationJob(Brawler::JobGroup& psoCompileGroup, std::array<HRESULT, std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR)>& compileResultsArr)
		{
			HRESULT& hr{ compileResultsArr[std::to_underlying(PSOIdentifier)] };

			psoCompileGroup.AddJob([this, &hr] ()
			{
				hr = CompilePSO<PSOIdentifier>();
			});

			if constexpr ((std::to_underlying(PSOIdentifier) + 1) != std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR))
				AddPSOCompilationJob<static_cast<Brawler::PSOs::PSOID>(std::to_underlying(PSOIdentifier) + 1)>(psoCompileGroup, compileResultsArr);
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		PSODatabase::PSODatabase() :
			mPSOMap()
		{
			// TODO: Ideally, we should have a system for streaming in PSOs as needed. For now,
			// however, we will just load them all at initialization time.

			std::array<HRESULT, std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR)> compileResultsArr{};

			Brawler::JobGroup psoCompileGroup{};
			psoCompileGroup.Reserve(std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR));

			AddPSOCompilationJob<static_cast<Brawler::PSOs::PSOID>(0)>(psoCompileGroup, compileResultsArr);

			psoCompileGroup.ExecuteJobs();

			for (const auto& compilationResult : compileResultsArr)
				CheckHRESULT(compilationResult);
		}

		PSODatabase& PSODatabase::GetInstance()
		{
			static PSODatabase instance{};
			return instance;
		}
	}
}