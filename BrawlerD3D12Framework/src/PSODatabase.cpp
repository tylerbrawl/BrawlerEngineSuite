module;
#include <array>
#include <cassert>
#include <filesystem>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.PSODatabase;
import Brawler.JobSystem;
import Util.Engine;
import Util.General;
import Brawler.PSOs.PSODefinition;

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
		HRESULT PSODatabase::CompilePSO()
		{
			auto psoStream{ Brawler::PSOs::CreatePSODescription<PSOIdentifier>() };
			const D3D12_PIPELINE_STATE_STREAM_DESC psoDesc{ 
				.SizeInBytes = sizeof(psoStream),
				.pPipelineStateSubobjectStream = reinterpret_cast<void*>(&psoStream)
			};

			// First try to load the PSO from the PSO library. If that succeeded, then we do
			// not have to spend ages (relatively speaking) creating it from scratch.
			{
				std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>> cachedPSO{ mPSOLibrary.TryLoadExistingPSO<PSOIdentifier>(psoDesc) };

				if (cachedPSO.has_value()) [[likely]]
				{
					mPSOMap[std::to_underlying(PSOIdentifier)] = std::move(*cachedPSO);
					return S_OK;
				}
			}

			// If that failed, then we have no choice but to create the PSO again.
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
		PSODatabase& PSODatabase::GetInstance()
		{
			static PSODatabase instance{};
			return instance;
		}

		void PSODatabase::InitializePSOLibrary()
		{
			mPSOLibrary.Initialize();
		}

		void PSODatabase::LoadPSOs()
		{
			// TODO: Ideally, we should have a system for streaming in PSOs as needed. For now,
			// however, we will just load them all at initialization time.
			//
			// UPDATE: In retrospect, this might actually be a bad idea. Elden Ring does the same
			// thing, apparently, and we all know how that went...

			std::array<HRESULT, std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR)> compileResultsArr{};

			Brawler::JobGroup psoCompileGroup{};
			psoCompileGroup.Reserve(std::to_underlying(Brawler::PSOs::PSOID::COUNT_OR_ERROR));

			AddPSOCompilationJob<static_cast<Brawler::PSOs::PSOID>(0)>(psoCompileGroup, compileResultsArr);

			psoCompileGroup.ExecuteJobs();

			for (const auto& compilationResult : compileResultsArr)
				Util::General::CheckHRESULT(compilationResult);

			// After all of the PSOs have been compiled, we can write the PSO library out to the
			// disk, should we need to. This is done asynchronously so as to not stall the runtime.
			mPSOLibrary.SerializePSOLibraryAsync();
		}
	}
}