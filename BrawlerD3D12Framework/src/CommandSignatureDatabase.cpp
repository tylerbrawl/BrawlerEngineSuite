module;
#include <array>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.CommandSignatureDatabase;
import Brawler.CommandSignatures.CommandSignatureDefinition;
import Util.Engine;
import Util.General;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.RootSignatures.RootSignatureID;
import Brawler.JobSystem;

namespace
{
	using CommandSignatureID = Brawler::CommandSignatures::CommandSignatureID;

	template <CommandSignatureID CSIdentifier>
	HRESULT CreateCommandSignature(Microsoft::WRL::ComPtr<Brawler::D3D12CommandSignature>& cmdSignaturePtr)
	{
		static_assert(CSIdentifier != CommandSignatureID::COUNT_OR_ERROR, "ERROR: An invalid CommandSignatureID was specified in a call to <anonymous namespace>::CreateCommandSignature()! (See CommandSignatureDatabase.cpp.)");

		static constexpr bool DOES_COMMAND_SIGNATURE_HAVE_ROOT_SIGNATURE = Brawler::CommandSignatures::DoesCommandSignatureHaveAssociatedRootSignature<CSIdentifier>();
		static constexpr D3D12_COMMAND_SIGNATURE_DESC COMMAND_SIGNATURE_DESC{ Brawler::CommandSignatures::CreateCommandSignatureDescription<CSIdentifier>() };

		// Only associate a root signature with a command signature if we need to. That way, we
		// can potentially reuse some command signatures for situations in which different root
		// signatures are current set on a command list.
		//
		// We determine whether or not we can do this at compile time. If we need to associate
		// the command signature with a root signature, then the corresponding RootSignatureID
		// is also found at compile time.
		if constexpr (DOES_COMMAND_SIGNATURE_HAVE_ROOT_SIGNATURE)
		{
			static constexpr Brawler::RootSignatures::RootSignatureID ROOT_SIGNATURE_ID = Brawler::CommandSignatures::GetRootSignatureForCommandSignature<CSIdentifier>();

			Brawler::D3D12RootSignature& relevantRootSignature{ Brawler::D3D12::RootSignatureDatabase::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>() };
			return Util::Engine::GetD3D12Device().CreateCommandSignature(&COMMAND_SIGNATURE_DESC, &relevantRootSignature, IID_PPV_ARGS(&cmdSignaturePtr));
		}
		else
			return Util::Engine::GetD3D12Device().CreateCommandSignature(&COMMAND_SIGNATURE_DESC, nullptr, IID_PPV_ARGS(&cmdSignaturePtr));
	}
}

namespace Brawler
{
	namespace D3D12
	{
		CommandSignatureDatabase& CommandSignatureDatabase::GetInstance()
		{
			static CommandSignatureDatabase instance{};
			return instance;
		}

		void CommandSignatureDatabase::InitializeDatabase()
		{
			// The D3D12 Create* APIs are thread safe, so we can create all of the command signatures
			// concurrently. (In fact, we overlap command signature creation with PSO compilation.)
			Brawler::JobGroup cmdSignatureCreationGroup{};
			cmdSignatureCreationGroup.Reserve(std::to_underlying(CommandSignatureID::COUNT_OR_ERROR));

			std::array<HRESULT, std::to_underlying(CommandSignatureID::COUNT_OR_ERROR)> creationResultsArr{};

			const auto addCreationJobLambda = [this, &cmdSignatureCreationGroup, &creationResultsArr]<CommandSignatureID CSIdentifier>(this const auto& self)
			{
				if constexpr (CSIdentifier != CommandSignatureID::COUNT_OR_ERROR)
				{
					static constexpr std::underlying_type_t<CommandSignatureID> CURR_COMMAND_SIGNATURE_INDEX = std::to_underlying(CSIdentifier);

					Microsoft::WRL::ComPtr<Brawler::D3D12CommandSignature>& currCmdSignaturePtr{ mCSPtrArr[CURR_COMMAND_SIGNATURE_INDEX] };
					HRESULT& currHResult{ creationResultsArr[CURR_COMMAND_SIGNATURE_INDEX] };

					cmdSignatureCreationGroup.AddJob([&currCmdSignaturePtr, &currHResult] ()
					{
						currHResult = CreateCommandSignature<CSIdentifier>(currCmdSignaturePtr);
					});

					static constexpr CommandSignatureID NEXT_ID = static_cast<CommandSignatureID>(CURR_COMMAND_SIGNATURE_INDEX + 1);
					self.template operator()<NEXT_ID>();
				}
			};

			addCreationJobLambda.template operator()<static_cast<CommandSignatureID>(0)>();

			for (const auto creationResult : creationResultsArr)
				Util::General::CheckHRESULT(creationResult);
		}
	}
}