module;
#include <array>
#include <cassert>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.RootSignatureDatabase;
import Brawler.JobSystem;
import Util.Engine;
import Util.General;
import Brawler.RootSignatures.RootSignatureDefinition;

namespace
{
	template <Brawler::RootSignatures::RootSignatureID RSIdentifier, D3D_ROOT_SIGNATURE_VERSION HighestVersion>
	__forceinline void AddRootSignatureCreationJob(std::array<Microsoft::WRL::ComPtr<Brawler::D3D12RootSignature>, std::to_underlying(decltype(RSIdentifier)::COUNT_OR_ERROR)>& rsArray, Brawler::JobGroup& jobGroup)
	{
		Microsoft::WRL::ComPtr<Brawler::D3D12RootSignature>& rootSigObject{ rsArray[std::to_underlying(RSIdentifier)] };
		jobGroup.AddJob([&rootSigObject] ()
		{
			if constexpr (std::to_underlying(HighestVersion) >= D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_1)
			{
				constexpr auto rootSignatureSpan{ Brawler::RootSignatures::GetSerializedRootSignature1_1<RSIdentifier>() };
				Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateRootSignature(
					0,
					rootSignatureSpan.data(),
					rootSignatureSpan.size_bytes(),
					IID_PPV_ARGS(&rootSigObject)
				));
			}

			if constexpr (HighestVersion == D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0)
			{
				constexpr auto rootSignatureSpan{ Brawler::RootSignatures::GetSerializedRootSignature1_0<RSIdentifier>() };
				Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateRootSignature(
					0,
					rootSignatureSpan.data(),
					rootSignatureSpan.size_bytes(),
					IID_PPV_ARGS(&rootSigObject)
				));
			}
		});

		if constexpr ((std::to_underlying(RSIdentifier) + 1) != std::to_underlying(decltype(RSIdentifier)::COUNT_OR_ERROR))
			AddRootSignatureCreationJob<static_cast<Brawler::RootSignatures::RootSignatureID>(std::to_underlying(RSIdentifier) + 1), HighestVersion>(rsArray, jobGroup);
	}
}

namespace Brawler
{
	namespace D3D12
	{
		RootSignatureDatabase::RootSignatureDatabase() :
			mRootSigMap()
		{
			// Root signature objects should not take long at all to compile (TODO: source?). Thus,
			// we will just compile them all at once.

			// First, we check for root signature 1.1 support.
			D3D_ROOT_SIGNATURE_VERSION highestSupportedVersion = D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0;

			{
				D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSigFeatureData{};
				const HRESULT hr = Util::Engine::GetD3D12Device().CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_ROOT_SIGNATURE, &rootSigFeatureData, sizeof(rootSigFeatureData));

				if (SUCCEEDED(hr)) [[likely]]
					highestSupportedVersion = rootSigFeatureData.HighestVersion;
			}

			Brawler::JobGroup rootSigCreationJobGroup{};
			rootSigCreationJobGroup.Reserve(std::to_underlying(Brawler::RootSignatures::RootSignatureID::COUNT_OR_ERROR));

			if (std::to_underlying(highestSupportedVersion) >= std::to_underlying(D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_1)) [[likely]]
				AddRootSignatureCreationJob<static_cast<Brawler::RootSignatures::RootSignatureID>(0), D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_1>(mRootSigMap, rootSigCreationJobGroup);
			else
				AddRootSignatureCreationJob<static_cast<Brawler::RootSignatures::RootSignatureID>(0), D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0>(mRootSigMap, rootSigCreationJobGroup);

			rootSigCreationJobGroup.ExecuteJobs();
		}

		RootSignatureDatabase& RootSignatureDatabase::GetInstance()
		{
			static RootSignatureDatabase instance{};
			return instance;
		}
	}
}