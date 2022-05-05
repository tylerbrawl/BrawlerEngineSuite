module;
#include <array>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.PSODatabase;
import Brawler.JobSystem;
import Util.Engine;

export namespace Brawler
{
	namespace D3D12
	{
		// PSOIdentifierEnumType should be Brawler::PSOs::PSOID.
		template <typename PSOIdentifierEnumType>
		class PSODatabase
		{
		private:
			PSODatabase();

		public:
			~PSODatabase() = default;

			PSODatabase(const PSODatabase& rhs) = delete;
			PSODatabase& operator=(const PSODatabase& rhs) = delete;

			PSODatabase(PSODatabase&& rhs) noexcept = default;
			PSODatabase& operator=(PSODatabase&& rhs) noexcept = default;
			
			static PSODatabase& GetInstance();

			template <PSOIdentifierEnumType PSOIdentifier>
				requires (PSOIdentifier != PSOIdentifierEnumType::COUNT_OR_ERROR)
			Brawler::D3D12PipelineState& GetPipelineState();

			template <PSOIdentifierEnumType PSOIdentifier>
				requires (PSOIdentifier != PSOIdentifierEnumType::COUNT_OR_ERROR)
			const Brawler::D3D12PipelineState& GetPipelineState() const;

		private:
			template <PSOIdentifierEnumType PSOIdentifer>
			HRESULT CompilePSO();

			template <PSOIdentifierEnumType PSOIdentifier>
			void AddPSOCompilationJob(Brawler::JobGroup& psoCompileGroup, std::array<HRESULT, std::to_underlying(PSOIdentifierEnumType::COUNT_OR_ERROR)>& compileResultsArr);

		private:
			std::array<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>, std::to_underlying(PSOIdentifierEnumType::COUNT_OR_ERROR)> mPSOMap;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace PSOs
	{
		// This function is defined in the .ixx files generated by the Brawler Shader Compiler.
		
		template <auto PSOIdentifier>
		extern auto CreatePSODescription();
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <typename PSOIdentifierEnumType>
		PSODatabase<PSOIdentifierEnumType>::PSODatabase() :
			mPSOMap()
		{
			// TODO: Ideally, we should have a system for streaming in PSOs as needed. For now,
			// however, we will just load them all at initialization time.

			std::array<HRESULT, std::to_underlying(PSOIdentifierEnumType::COUNT_OR_ERROR)> compileResultsArr{};

			Brawler::JobGroup psoCompileGroup{};
			psoCompileGroup.Reserve(std::to_underlying(PSOIdentifierEnumType::COUNT_OR_ERROR));

			AddPSOCompilationJob<static_cast<PSOIdentifierEnumType>(0)>(psoCompileGroup, compileResultsArr);

			psoCompileGroup.ExecuteJobs();

			for (const auto& compilationResult : compileResultsArr)
				CheckHRESULT(compilationResult);
		}

		template <typename PSOIdentifierEnumType>
		PSODatabase<PSOIdentifierEnumType>& PSODatabase<PSOIdentifierEnumType>::GetInstance()
		{
			static PSODatabase<PSOIdentifierEnumType> instance{};
			return instance;
		}
		
		template <typename PSOIdentifierEnumType>
		template <PSOIdentifierEnumType PSOIdentifier>
			requires (PSOIdentifier != PSOIdentifierEnumType::COUNT_OR_ERROR)
		Brawler::D3D12PipelineState& PSODatabase<PSOIdentifierEnumType>::GetPipelineState()
		{
			assert(mPSOMap[std::to_underlying(PSOIdentifier)] != nullptr && "ERROR: An attempt was made to access a PSO in a PSODatabase before it could be compiled!");
			return *(mPSOMap[std::to_underlying(PSOIdentifier)].Get());
		}

		template <typename PSOIdentifierEnumType>
		template <PSOIdentifierEnumType PSOIdentifier>
			requires (PSOIdentifier != PSOIdentifierEnumType::COUNT_OR_ERROR)
		const Brawler::D3D12PipelineState& PSODatabase<PSOIdentifierEnumType>::GetPipelineState() const
		{
			assert(mPSOMap[std::to_underlying(PSOIdentifier)] != nullptr && "ERROR: An attempt was made to access a PSO in a PSODatabase before it could be compiled!");
			return *(mPSOMap[std::to_underlying(PSOIdentifier)].Get());
		}

		template <typename PSOIdentifierEnumType>
		template <PSOIdentifierEnumType PSOIdentifier>
		HRESULT PSODatabase<PSOIdentifierEnumType>::CompilePSO()
		{
			// TODO: Create some way to retrieve cached PSOs. Alternatively (and preferably), we
			// could edit the generated PSODefinition::ExecuteRuntimePSOResolution() function to
			// do this for us.

			auto psoStream{ Brawler::PSOs::CreatePSODescription<PSOIdentifier>() };
			const D3D12_PIPELINE_STATE_STREAM_DESC psoStreamDesc{
				.SizeInBytes = sizeof(psoStream),
				.pPipelineStateSubobjectStream = &psoStream
			};

			return Util::Engine::GetD3D12Device().CreatePipelineState(&psoStreamDesc, IID_PPV_ARGS(&(mPSOMap[std::to_underlying(PSOIdentifier)])));
		}

		template <typename PSOIdentifierEnumType>
		template <PSOIdentifierEnumType PSOIdentifier>
		void PSODatabase<PSOIdentifierEnumType>::AddPSOCompilationJob(Brawler::JobGroup& psoCompileGroup, std::array<HRESULT, std::to_underlying(PSOIdentifierEnumType::COUNT_OR_ERROR)>& compileResultsArr)
		{
			HRESULT& hr{ compileResultsArr[std::to_underlying(PSOIdentifier)] };

			psoCompileGroup.AddJob([this, &hr] ()
			{
				hr = CompilePSO<PSOIdentifier>();
			});

			if constexpr ((std::to_underlying(PSOIdentifier) + 1) != std::to_underlying(PSOIdentifierEnumType::COUNT_OR_ERROR))
				AddPSOCompilationJob<static_cast<PSOIdentifierEnumType>(std::to_underlying(PSOIdentifier) + 1)>(psoCompileGroup, compileResultsArr);
		}
	}
}